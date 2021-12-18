// Copyright 2021 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

/* eslint-env browser, jasmine */
import 'jasmine';

import {
  Channel,
  Client,
  decode,
  MethodStub,
  ServiceClient,
} from '@pigweed/pw_rpc';
import {Status} from '@pigweed/pw_status';
import {
  PacketType,
  RpcPacket,
} from 'packet_proto_tspb/packet_proto_tspb_pb/pw_rpc/internal/packet_pb';
import {ProtoCollection} from 'transfer_proto_collection/generated/ts_proto_collection';
import {Chunk} from 'transfer_proto_tspb/transfer_proto_tspb_pb/pw_transfer/transfer_pb';

import {Manager} from './client';
import {ProgressStats} from './transfer';

const DEFAULT_TIMEOUT_S = 0.3;

describe('Encoder', () => {
  const textEncoder = new TextEncoder();
  const textDecoder = new TextDecoder();
  let client: Client;
  let service: ServiceClient;
  let sentChunks: Chunk[];
  let packetsToSend: Uint8Array[][];

  beforeEach(() => {
    const lib = new ProtoCollection();
    const channels: Channel[] = [new Channel(1, handleRequest)];
    client = Client.fromProtoSet(channels, lib);
    service = client.channel(1)!.service('pw.transfer.Transfer')!;

    sentChunks = [];
    packetsToSend = [];
  });

  function handleRequest(data: Uint8Array): void {
    const packet = decode(data);
    if (packet.getType() !== PacketType.CLIENT_STREAM) {
      return;
    }

    const chunk = Chunk.deserializeBinary(packet.getPayload_asU8());
    sentChunks.push(chunk);

    if (packetsToSend.length > 0) {
      const responses = packetsToSend.shift()!;
      for (const response of responses) {
        client.processPacket(response);
      }
    }
  }

  function receivedData(): Uint8Array {
    let length = 0;
    sentChunks.forEach((chunk: Chunk) => {
      length += chunk.getData().length;
    });
    const data = new Uint8Array(length);
    let offset = 0;
    sentChunks.forEach((chunk: Chunk) => {
      data.set(chunk.getData() as Uint8Array, offset);
      offset += chunk.getData().length;
    });
    return data;
  }

  function enqueueServerError(method: MethodStub, error: Status): void {
    const packet = new RpcPacket();
    packet.setType(PacketType.SERVER_ERROR);
    packet.setChannelId(1);
    packet.setServiceId(service.id);
    packet.setMethodId(method.id);
    packet.setStatus(error);
    packetsToSend.push([packet.serializeBinary()]);
  }

  function enqueueServerResponses(method: MethodStub, responses: Chunk[][]) {
    for (const responseGroup of responses) {
      const serializedGroup = [];
      for (const response of responseGroup) {
        const packet = new RpcPacket();
        packet.setType(PacketType.SERVER_STREAM);
        packet.setChannelId(1);
        packet.setServiceId(service.id);
        packet.setMethodId(method.id);
        packet.setStatus(Status.OK);
        packet.setPayload(response.serializeBinary());
        serializedGroup.push(packet.serializeBinary());
      }
      packetsToSend.push(serializedGroup);
    }
  }

  function buildChunk(
    transferId: number,
    offset: number,
    data: string,
    remainingBytes: number
  ): Chunk {
    const chunk = new Chunk();
    chunk.setTransferId(transferId);
    chunk.setOffset(offset);
    chunk.setData(textEncoder.encode(data));
    chunk.setRemainingBytes(remainingBytes);
    return chunk;
  }

  it('read transfer basic', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    const chunk1 = buildChunk(3, 0, 'abc', 0);
    enqueueServerResponses(service.method('Read')!, [[chunk1]]);

    const data = await manager.read(3);
    expect(textDecoder.decode(data)).toEqual('abc');
    expect(sentChunks).toHaveSize(2);
    expect(sentChunks[sentChunks.length - 1].hasStatus()).toBeTrue();
    expect(sentChunks[sentChunks.length - 1].getStatus()).toEqual(Status.OK);
  });

  it('read transfer multichunk', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    const chunk1 = buildChunk(3, 0, 'abc', 3);
    const chunk2 = buildChunk(3, 3, 'def', 0);
    enqueueServerResponses(service.method('Read')!, [[chunk1, chunk2]]);

    const data = await manager.read(3);
    expect(data).toEqual(textEncoder.encode('abcdef'));
    expect(sentChunks).toHaveSize(2);
    expect(sentChunks[sentChunks.length - 1].hasStatus()).toBeTrue();
    expect(sentChunks[sentChunks.length - 1].getStatus()).toEqual(Status.OK);
  });

  it('read transfer progress callback', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    const chunk1 = buildChunk(3, 0, 'abc', 3);
    const chunk2 = buildChunk(3, 3, 'def', 0);
    enqueueServerResponses(service.method('Read')!, [[chunk1, chunk2]]);

    const progress: Array<ProgressStats> = [];

    const data = await manager.read(3, (stats: ProgressStats) => {
      progress.push(stats);
    });
    expect(textDecoder.decode(data)).toEqual('abcdef');
    expect(sentChunks).toHaveSize(2);
    expect(sentChunks[sentChunks.length - 1].hasStatus()).toBeTrue();
    expect(sentChunks[sentChunks.length - 1].getStatus()).toEqual(Status.OK);

    expect(progress).toEqual([
      new ProgressStats(3, 3, 6),
      new ProgressStats(6, 6, 6),
    ]);
  });

  it('read transfer retry bad offset', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    const chunk1 = buildChunk(3, 0, '123', 6);
    const chunk2 = buildChunk(3, 1, '456', 3); // Incorrect offset; expecting 3
    const chunk3 = buildChunk(3, 3, '456', 3);
    const chunk4 = buildChunk(3, 6, '789', 0);

    enqueueServerResponses(service.method('Read')!, [
      [chunk1, chunk2],
      [chunk3, chunk4],
    ]);

    const data = await manager.read(3);
    expect(data).toEqual(textEncoder.encode('123456789'));
    expect(sentChunks).toHaveSize(3);
    expect(sentChunks[sentChunks.length - 1].hasStatus()).toBeTrue();
    expect(sentChunks[sentChunks.length - 1].getStatus()).toEqual(Status.OK);
  });

  it('read transfer retry timeout', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    const chunk = buildChunk(3, 0, 'xyz', 0);
    enqueueServerResponses(service.method('Read')!, [[], [chunk]]);

    const data = await manager.read(3);
    expect(textDecoder.decode(data)).toEqual('xyz');

    // Two transfer parameter requests should have been sent.
    expect(sentChunks).toHaveSize(3);
    expect(sentChunks[sentChunks.length - 1].hasStatus()).toBeTrue();
    expect(sentChunks[sentChunks.length - 1].getStatus()).toEqual(Status.OK);
  });

  it('read transfer timeout', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    await manager
      .read(27)
      .then(() => {
        fail('Unexpected completed promise');
      })
      .catch(error => {
        expect(error.id).toEqual(27);
        expect(Status[error.status]).toEqual(Status[Status.DEADLINE_EXCEEDED]);
        expect(sentChunks).toHaveSize(4);
      });
  });

  it('read transfer error', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    const chunk = new Chunk();
    chunk.setStatus(Status.NOT_FOUND);
    chunk.setTransferId(31);
    enqueueServerResponses(service.method('Read')!, [[chunk]]);

    await manager
      .read(31)
      .then(() => {
        fail('Unexpected completed promise');
      })
      .catch(error => {
        expect(error.id).toEqual(31);
        expect(Status[error.status]).toEqual(Status[Status.NOT_FOUND]);
      });
  });

  it('read transfer server error', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    enqueueServerError(service.method('Read')!, Status.NOT_FOUND);
    await manager
      .read(31)
      .then(data => {
        fail('Unexpected completed promise');
      })
      .catch(error => {
        expect(error.id).toEqual(31);
        expect(Status[error.status]).toEqual(Status[Status.INTERNAL]);
      });
  });

  it('write transfer basic', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    const chunk = new Chunk();
    chunk.setTransferId(4);
    chunk.setOffset(0);
    chunk.setPendingBytes(32);
    chunk.setMaxChunkSizeBytes(8);

    const completeChunk = new Chunk();
    completeChunk.setTransferId(4);
    completeChunk.setStatus(Status.OK);

    enqueueServerResponses(service.method('Write')!, [
      [chunk],
      [completeChunk],
    ]);

    await manager.write(4, textEncoder.encode('hello'));
    expect(sentChunks).toHaveSize(2);
    expect(receivedData()).toEqual(textEncoder.encode('hello'));
  });

  it('write transfer max chunk size', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    const chunk = new Chunk();
    chunk.setTransferId(4);
    chunk.setOffset(0);
    chunk.setPendingBytes(32);
    chunk.setMaxChunkSizeBytes(8);

    const completeChunk = new Chunk();
    completeChunk.setTransferId(4);
    completeChunk.setStatus(Status.OK);

    enqueueServerResponses(service.method('Write')!, [
      [chunk],
      [completeChunk],
    ]);

    await manager.write(4, textEncoder.encode('hello world'));
    expect(sentChunks).toHaveSize(3);
    expect(receivedData()).toEqual(textEncoder.encode('hello world'));
    expect(sentChunks[1].getData()).toEqual(textEncoder.encode('hello wo'));
    expect(sentChunks[2].getData()).toEqual(textEncoder.encode('rld'));
  });

  it('write transfer multiple parameters', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    const chunk = new Chunk();
    chunk.setTransferId(4);
    chunk.setOffset(0);
    chunk.setPendingBytes(8);
    chunk.setMaxChunkSizeBytes(8);

    const chunk2 = new Chunk();
    chunk2.setTransferId(4);
    chunk2.setOffset(8);
    chunk2.setPendingBytes(8);
    chunk2.setMaxChunkSizeBytes(8);

    const completeChunk = new Chunk();
    completeChunk.setTransferId(4);
    completeChunk.setStatus(Status.OK);

    enqueueServerResponses(service.method('Write')!, [
      [chunk],
      [chunk2],
      [completeChunk],
    ]);

    await manager.write(4, textEncoder.encode('data to write'));
    expect(sentChunks).toHaveSize(3);
    expect(receivedData()).toEqual(textEncoder.encode('data to write'));
    expect(sentChunks[1].getData()).toEqual(textEncoder.encode('data to '));
    expect(sentChunks[2].getData()).toEqual(textEncoder.encode('write'));
  });

  it('write transfer progress callback', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    const chunk = new Chunk();
    chunk.setTransferId(4);
    chunk.setOffset(0);
    chunk.setPendingBytes(8);
    chunk.setMaxChunkSizeBytes(8);

    const chunk2 = new Chunk();
    chunk2.setTransferId(4);
    chunk2.setOffset(8);
    chunk2.setPendingBytes(8);
    chunk2.setMaxChunkSizeBytes(8);

    const completeChunk = new Chunk();
    completeChunk.setTransferId(4);
    completeChunk.setStatus(Status.OK);

    enqueueServerResponses(service.method('Write')!, [
      [chunk],
      [chunk2],
      [completeChunk],
    ]);

    const progress: Array<ProgressStats> = [];
    await manager.write(
      4,
      textEncoder.encode('data to write'),
      (stats: ProgressStats) => {
        progress.push(stats);
      }
    );
    expect(sentChunks).toHaveSize(3);
    expect(receivedData()).toEqual(textEncoder.encode('data to write'));
    expect(sentChunks[1].getData()).toEqual(textEncoder.encode('data to '));
    expect(sentChunks[2].getData()).toEqual(textEncoder.encode('write'));

    console.log(progress);
    expect(progress).toEqual([
      new ProgressStats(8, 0, 13),
      new ProgressStats(13, 8, 13),
      new ProgressStats(13, 13, 13),
    ]);
  });

  it('write transfer rewind', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    const chunk1 = new Chunk();
    chunk1.setTransferId(4);
    chunk1.setOffset(0);
    chunk1.setPendingBytes(8);
    chunk1.setMaxChunkSizeBytes(8);

    const chunk2 = new Chunk();
    chunk2.setTransferId(4);
    chunk2.setOffset(8);
    chunk2.setPendingBytes(8);
    chunk2.setMaxChunkSizeBytes(8);

    const chunk3 = new Chunk();
    chunk3.setTransferId(4);
    chunk3.setOffset(4); // Rewind
    chunk3.setPendingBytes(8);
    chunk3.setMaxChunkSizeBytes(8);

    const chunk4 = new Chunk();
    chunk4.setTransferId(4);
    chunk4.setOffset(12); // Rewind
    chunk4.setPendingBytes(16);
    chunk4.setMaxChunkSizeBytes(16);

    const completeChunk = new Chunk();
    completeChunk.setTransferId(4);
    completeChunk.setStatus(Status.OK);

    enqueueServerResponses(service.method('Write')!, [
      [chunk1],
      [chunk2],
      [chunk3],
      [chunk4],
      [completeChunk],
    ]);

    await manager.write(4, textEncoder.encode('pigweed data transfer'));
    expect(sentChunks).toHaveSize(5);
    expect(sentChunks[1].getData()).toEqual(textEncoder.encode('pigweed '));
    expect(sentChunks[2].getData()).toEqual(textEncoder.encode('data tra'));
    expect(sentChunks[3].getData()).toEqual(textEncoder.encode('eed data'));
    expect(sentChunks[4].getData()).toEqual(textEncoder.encode(' transfer'));
  });

  it('write transfer bad offset', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    const chunk1 = new Chunk();
    chunk1.setTransferId(4);
    chunk1.setOffset(0);
    chunk1.setPendingBytes(8);
    chunk1.setMaxChunkSizeBytes(8);

    const chunk2 = new Chunk();
    chunk2.setTransferId(4);
    chunk2.setOffset(100); // larger offset than data
    chunk2.setPendingBytes(8);
    chunk2.setMaxChunkSizeBytes(8);

    const completeChunk = new Chunk();
    completeChunk.setTransferId(4);
    completeChunk.setStatus(Status.OK);

    enqueueServerResponses(service.method('Write')!, [
      [chunk1],
      [chunk2],
      [completeChunk],
    ]);

    await manager
      .write(4, textEncoder.encode('small data'))
      .then(() => {
        fail('Unexpected succesful promise');
      })
      .catch(error => {
        expect(error.id).toEqual(4);
        expect(Status[error.status]).toEqual(Status[Status.OUT_OF_RANGE]);
      });
  });

  it('write transfer error', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    const chunk = new Chunk();
    chunk.setTransferId(21);
    chunk.setStatus(Status.UNAVAILABLE);

    enqueueServerResponses(service.method('Write')!, [[chunk]]);

    await manager
      .write(21, textEncoder.encode('no write'))
      .then(() => {
        fail('Unexpected succesful promise');
      })
      .catch(error => {
        expect(error.id).toEqual(21);
        expect(Status[error.status]).toEqual(Status[Status.UNAVAILABLE]);
      });
  });

  it('write transfer server error', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    const chunk = new Chunk();
    chunk.setTransferId(21);
    chunk.setStatus(Status.NOT_FOUND);

    enqueueServerError(service.method('Write')!, Status.NOT_FOUND);

    await manager
      .write(21, textEncoder.encode('server error'))
      .then(() => {
        fail('Unexpected succesful promise');
      })
      .catch(error => {
        expect(error.id).toEqual(21);
        expect(Status[error.status]).toEqual(Status[Status.INTERNAL]);
      });
  });

  it('write transfer timeout after initial chunk', async () => {
    const manager = new Manager(service, 0.001, 4, 2);

    await manager
      .write(22, textEncoder.encode('no server response!'))
      .then(() => {
        fail('unexpected succesful write');
      })
      .catch(error => {
        expect(sentChunks).toHaveSize(3); // Initial chunk + two retries.
        expect(error.id).toEqual(22);
        expect(Status[error.status]).toEqual(Status[Status.DEADLINE_EXCEEDED]);
      });
  });

  it('write transfer timeout after intermediate chunk', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S, 4, 2);

    const chunk = new Chunk();
    chunk.setTransferId(22);
    chunk.setPendingBytes(10);
    chunk.setMaxChunkSizeBytes(5);

    enqueueServerResponses(service.method('Write')!, [[chunk]]);

    await manager
      .write(22, textEncoder.encode('0123456789'))
      .then(() => {
        fail('unexpected succesful write');
      })
      .catch(error => {
        const expectedChunk1 = new Chunk();
        expectedChunk1.setTransferId(22);
        const expectedChunk2 = new Chunk();
        expectedChunk2.setTransferId(22);
        expectedChunk2.setData(textEncoder.encode('01234'));
        const lastChunk = new Chunk();
        lastChunk.setTransferId(22);
        lastChunk.setData(textEncoder.encode('56789'));
        lastChunk.setOffset(5);
        lastChunk.setRemainingBytes(0);

        const expectedChunks = [
          expectedChunk1,
          expectedChunk2,
          lastChunk,
          lastChunk, // retry 1
          lastChunk, // retry 2
        ];

        expect(sentChunks).toEqual(expectedChunks);

        expect(error.id).toEqual(22);
        expect(Status[error.status]).toEqual(Status[Status.DEADLINE_EXCEEDED]);
      });
  });

  it('write zero pending bytes is internal error', async () => {
    const manager = new Manager(service, DEFAULT_TIMEOUT_S);

    const chunk = new Chunk();
    chunk.setTransferId(23);
    chunk.setPendingBytes(0);

    enqueueServerResponses(service.method('Write')!, [[chunk]]);

    await manager
      .write(23, textEncoder.encode('no write'))
      .then(() => {
        fail('Unexpected succesful promise');
      })
      .catch(error => {
        expect(error.id).toEqual(23);
        expect(Status[error.status]).toEqual(Status[Status.INTERNAL]);
      });
  });
});
