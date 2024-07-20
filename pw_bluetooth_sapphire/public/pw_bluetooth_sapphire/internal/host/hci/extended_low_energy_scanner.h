// Copyright 2024 The Pigweed Authors
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

#pragma once

#include "pw_bluetooth_sapphire/internal/host/hci/low_energy_scanner.h"
namespace bt::hci {

// ExtendedLowEnergyScanner implements the LowEnergyScanner interface for
// controllers that support the 5.0 Extended Advertising feature. This uses the
// extended HCI LE scan commands and events:
//
//     - HCI_LE_Set_Extended_Scan_Parameters
//     - HCI_LE_Set_Extended_Scan_Enable
//     - HCI_LE_Extended_Advertising_Report event
//
// After enabling scanning, zero or more HCI_LE_Extended_Advertising_Report
// events are generated by the Controller based on any advertising packets
// received and the duplicate filtering in effect. ExtendedLowEnergyAdvertiser
// subscribes to this event, parses the results, and returns discovered peers
// via the delegate.
//
// As currently implemented, this scanner uses a continuous scan duration and
// doesn't subscribe to the HCI_LE_Scan_Timeout Event.
class ExtendedLowEnergyScanner final : public LowEnergyScanner {
 public:
  ExtendedLowEnergyScanner(LocalAddressDelegate* local_addr_delegate,
                           Transport::WeakPtr transport,
                           pw::async::Dispatcher& pw_dispatcher);
  ~ExtendedLowEnergyScanner() override;

  bool StartScan(const ScanOptions& options,
                 ScanStatusCallback callback) override;

 private:
  // Build the HCI command packet to set the scan parameters for the flavor of
  // low energy scanning being implemented.
  EmbossCommandPacket BuildSetScanParametersPacket(
      const DeviceAddress& local_address, const ScanOptions& options) override;

  // Build the HCI command packet to enable scanning for the flavor of low
  // energy scanning being implemented.
  EmbossCommandPacket BuildEnablePacket(
      const ScanOptions& options,
      pw::bluetooth::emboss::GenericEnableParam enable) override;

  // Parse out all the advertising reports that came in an HCI LE Extended
  // Advertising Report.
  static std::vector<pw::bluetooth::emboss::LEExtendedAdvertisingReportDataView>
  ParseAdvertisingReports(const EmbossEventPacket& event);

  // Event handler for HCI LE Extended Advertising Report event.
  void OnExtendedAdvertisingReportEvent(const EmbossEventPacket& event);

  // Our event handler ID for the LE Extended Advertising Report event.
  CommandChannel::EventHandlerId event_handler_id_;

  BT_DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(ExtendedLowEnergyScanner);
};

}  // namespace bt::hci
