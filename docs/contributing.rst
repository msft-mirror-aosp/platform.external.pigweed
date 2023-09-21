.. _docs-contributing:

============
Contributing
============
We'd love to accept your patches and contributions to Pigweed. There are just a
few small guidelines you need to follow.

Before participating in our community, please take a moment to review our
:ref:`docs-code-of-conduct`. We expect everyone who interacts with the project
to respect these guidelines.

Pigweed contribution overview
-----------------------------
.. note::

  If you have any trouble with this flow, reach out in our `chat room
  <https://discord.gg/M9NSeTA>`_ or on the `mailing list
  <https://groups.google.com/forum/#!forum/pigweed>`_ for help.

One-time contributor setup
^^^^^^^^^^^^^^^^^^^^^^^^^^
#. Sign the
   `Contributor License Agreement <https://cla.developers.google.com/>`_.
#. Verify that your Git user email (git config user.email) is either Google
   Account email or an Alternate email for the Google account used to sign
   the CLA (Manage Google account → Personal Info → email).
#. Obtain a login cookie from Gerrit's
   `new-password <https://pigweed.googlesource.com/new-password>`_ page.
#. Install the :ref:`gerrit-commit-hook` to automatically add a
   ``Change-Id: ...`` line to your commit.
#. Install the Pigweed presubmit check hook with ``pw presubmit --install``.
   Remember to :ref:`activate-pigweed-environment` first!

Presubmission process
^^^^^^^^^^^^^^^^^^^^^
Before making or sending major changes or SEEDs, please reach out in our
`chat room <https://discord.gg/M9NSeTA>`_ or on the `mailing list
<https://groups.google.com/forum/#!forum/pigweed>`_ first to ensure the changes
make sense for upstream. We generally go through a design phase before making
large changes. See :ref:`SEED-0001` for a description of this process; but
please discuss with us before writing a full SEED. Let us know of any
priorities, timelines, requirements, and limitations ahead of time.

For minor changes that don't fit the SEED process, follow the `Small changes`_
guidance and the `Change submission process`_.

.. warning::
   Skipping communicating with us before doing large amounts of work risks
   accepting your contribution. Communication is key!

Change submission process
^^^^^^^^^^^^^^^^^^^^^^^^^

.. note::
   A change is a single git commit, and is also known as Change List or CL for
   short.

.. tip::
   Follow `Small changes`_ for a smooth submission process.

#. Go through the `Presubmission process`_ and review this document's guidance.
#. Ensure all files include the correct copyright and license headers.
#. Include any necessary changes to the documentation.
#. Run :ref:`module-pw_presubmit` to detect style or compilation issues before
   uploading.
#. Upload the change with ``git push origin HEAD:refs/for/main``.
#. Add ``gwsq-pigweed@pigweed.google.com.iam.gserviceaccount.com`` as a
   reviewer. This will automatically choose an appropriate person to review the
   change.
#. Address any reviewer feedback by amending the commit
   (``git commit --amend``).
#. Submit change to CI builders to merge. If you are not part of Pigweed's
   core team, you can ask the reviewer to add the `+2 CQ` vote, which will
   trigger a rebase and submit once the builders pass.

Contributor License Agreement
-----------------------------
Contributions to this project must be accompanied by a Contributor License
Agreement. You (or your employer) retain the copyright to your contribution;
this simply gives us permission to use and redistribute your contributions as
part of the project. Head over to <https://cla.developers.google.com/> to see
your current agreements on file or to sign a new one.

You generally only need to submit a CLA once, so if you've already submitted one
(even if it was for a different project), you probably don't need to do it
again.

.. _gerrit-commit-hook:

Gerrit Commit Hook
------------------
Gerrit requires all changes to have a ``Change-Id`` tag at the bottom of each
commit message. You should set this up to be done automatically using the
instructions below.

The commands below assume that your current working directory is the root
of your Pigweed repository.

**Linux/macOS**

.. code-block:: bash

   f=`git rev-parse --git-dir`/hooks/commit-msg ; mkdir -p $(dirname $f) ; curl -Lo $f https://gerrit-review.googlesource.com/tools/hooks/commit-msg ; chmod +x $f

**Windows**

.. code-block:: batch

   git rev-parse --git-dir > gitrepopath.txt & set /p "g="< gitrepopath.txt & del gitrepopath.txt & call set "f=%g%/hooks" & call mkdir "%f%" & call curl -Lo "%f%/commit-msg" https://gerrit-review.googlesource.com/tools/hooks/commit-msg

Commit Message
--------------
See the :ref:`commit message section of the style guide<commit-style>` for how
commit messages should look.

Documentation
-------------
Most changes to Pigweed should have an associated documentation change.

Building
^^^^^^^^
To build the documentation, follow the :ref:`getting
started<docs-getting-started>` guide so you can build Pigweed. Then:

#. Change to your checkout directory and ``. activate.sh`` if necessary
#. Run ``pw watch out`` to build the code, run tests, and build docs
#. Wait for the build to finish (see a ``PASS``)
#. Navigate to  ``<CHECKOUT>/out/docs/gen/docs/html/index.html``
#. Edit the relevant ``.rst`` file. Save when ready
#. Refresh your browser after the build completes

Alternately, you can use the local webserver in watch; this works better for
some pages that have external resources: ``pw watch --serve-docs`` then
navigate to `http://localhost:8000 <http://localhost:8000>`_ in your browser.

Submission checklist
^^^^^^^^^^^^^^^^^^^^
All Pigweed changes must either:

#. Include updates to documentation, or
#. Include ``No-Docs-Update-Reason: <reason>`` in a Gerrit comment on the CL.
   For example:

   * ``No-Docs-Update-Reason: formatting tweaks``
   * ``No-Docs-Update-Reason: internal cleanups``
   * ``No-Docs-Update-Reason: bugfix``

It's acceptable to only document new changes in an otherwise underdocumented
module, but it's not acceptable to not document new changes because the module
doesn't have any other documentation.

Code Reviews
------------
All Pigweed development happens on Gerrit, following the `typical Gerrit
development workflow <http://ceres-solver.org/contributing.html>`_. Consult the
`Gerrit User Guide
<https://gerrit-documentation.storage.googleapis.com/Documentation/2.12.3/intro-user.html>`_
for more information on using Gerrit.

You may add the special address
``gwsq-pigweed@pigweed.google.com.iam.gserviceaccount.com`` as a reviewer to
have Gerrit automatically choose an appropriate person to review your change.

In the future we may support GitHub pull requests, but until that time we will
close GitHub pull requests and ask that the changes be uploaded to Gerrit
instead.

Small changes
^^^^^^^^^^^^^
Small changes are encouraged because they:

* Get reviewed more quickly, easily, and thoroughly.
* Are less likely to introduce bugs.
* Create less wasted work if they are rejected.
* Are easier to merge.
* Are easier to design well.
* Block work less since work can continued while the change is reviewed.
* Are simpler to roll back.

A small change can be defined as one self-contained change, which means it
makes a mininimal change addressing one thing, without breaking other users. It
is not necessarily defined by the number of lines changed. However, as a rule of
thumb, 100 changed lines is considered a small change while 1000 lines is too
large.

A small change includes related tests, sample usage, and documentation where
applicable. It includes everything the reviewer needs to understand the change
except for future development.

While reviewers have the discretion to reject a large change, you can work with
your reviewer to figure out how to proceed breaking changes apart.

Large changes aren't bad when deleting or moving files, moving large contents
without modifications, adding generated code, or an agreement is reached with
the reviewer in advance.

.. tip::
   One way to split up a large change is to split it up by groupings of files
   in smaller but self-contained changes. See `Splitting by Files
   <https://google.github.io/eng-practices/review/developer/small-cls.html#splitting-files>`_
   for examples.

.. tip::
   Separating functional changes from formatting cleanup changes helps create
   small changes.

.. tip::
   Separating refactoring from features and bug fixes also helps create small
   changes.

Read `Google's Eng-Practices Small CLs
<https://google.github.io/eng-practices/review/developer/small-cls.html>`_ for
more details.

Instructions for reviewers
^^^^^^^^^^^^^^^^^^^^^^^^^^
#.  Get the `Gerrit Monitor extension
    <https://chrome.google.com/webstore/detail/gerrit-monitor/leakcdjcdifiihdgalplgkghidmfafoh?hl=en>`_.
#.  When added to the attention set for a change, respond within 1 business day:

    * Review the change if possible, OR
    * If you will not be able to review the change within 1 business day (e.g.
      due to handling P0s), comment on the change stating so, and reassign to
      another reviewer if possible.
    * The response time expectation only applies to Googlers working full-time
      on Pigweed.
#. Remove yourself from the `attention set
   <https://gerrit-review.googlesource.com/Documentation/user-attention-set.html>`_
   for changes where another person (author or reviewer) must take action
   before you can continue to review. You are encouraged, but not required, to
   leave a comment when doing so, especially for changes by external
   contributors who may not be familiar with our process.

SLO
^^^
90% of changes on which a Googler working on Pigweed full-time is added to the
attention set as a reviewer get triaged within 1 business day.

Community Guidelines
--------------------
This project follows `Google's Open Source Community Guidelines
<https://opensource.google/conduct/>`_ and the :ref:`docs-code-of-conduct`.

Source Code Headers
-------------------
Every Pigweed file containing source code must include copyright and license
information. This includes any JS/CSS files that you might be serving out to
browsers.

Apache header for C and C++ files:

.. code-block:: none

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

Apache header for Python and GN files:

.. code-block:: none

  # Copyright 2020 The Pigweed Authors
  #
  # Licensed under the Apache License, Version 2.0 (the "License"); you may not
  # use this file except in compliance with the License. You may obtain a copy of
  # the License at
  #
  #     https://www.apache.org/licenses/LICENSE-2.0
  #
  # Unless required by applicable law or agreed to in writing, software
  # distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
  # WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
  # License for the specific language governing permissions and limitations under
  # the License.

Presubmit Checks and Continuous Integration
-------------------------------------------
All Pigweed change lists (CLs) must adhere to Pigweed's style guide and pass a
suite of automated builds, tests, and style checks to be merged upstream. Much
of this checking is done using Pigweed's ``pw_presubmit`` module by automated
builders. These builders run before each Pigweed CL is submitted and in our
continuous integration infrastructure (see `Pigweed's build console
<https://ci.chromium.org/p/pigweed/g/pigweed/console>`_).

Running Presubmit Checks
------------------------
To run automated presubmit checks on a pending CL, click the ``CQ DRY RUN``
button in the Gerrit UI. The results appear in the Tryjobs section, below the
source listing. Jobs that passed are green; jobs that failed are red.

If all checks pass, you will see a ``Dry run: This CL passed the CQ dry run.``
comment on your change. If any checks fail, you will see a ``Dry run: Failed
builds:`` message. All failures must be addressed before submitting.

In addition to the publicly visible presubmit checks, Pigweed runs internal
presubmit checks that are only visible within Google. If any these checks fail,
external developers will see a ``Dry run: Failed builds:`` comment on the CL,
even if all visible checks passed. Reach out to the Pigweed team for help
addressing these issues.

Project Presubmit Checks
------------------------
In addition to Pigweed's presubmit checks, some projects that use Pigweed run
their presubmit checks in Pigweed's infrastructure. This supports a development
flow where projects automatically update their Pigweed submodule if their tests
pass. If a project cannot build against Pigweed's tip-of-tree, it will stay on
a fixed Pigweed revision until the issues are fixed. See the `sample project
<https://pigweed.googlesource.com/pigweed/sample_project/>`_ for an example of
this.

Pigweed does its best to keep builds passing for dependent projects. In some
circumstances, the Pigweed maintainers may choose to merge changes that break
dependent projects. This will only be done if

* a feature or fix is needed urgently in Pigweed or for a different project,
  and
* the project broken by the change does not imminently need Pigweed updates.

The downstream project will continue to build against their last working
revision of Pigweed until the incompatibilities are fixed.

In these situations, Pigweed's commit queue submission process will fail for all
changes. If a change passes all presubmit checks except for known failures, the
Pigweed team may permit manual submission of the CL. Contact the Pigweed team
for submission approval.

Running local presubmits
------------------------
To speed up the review process, consider adding :ref:`module-pw_presubmit` as a
git push hook using the following command:

Linux/macOS
^^^^^^^^^^^
.. code-block:: bash

  $ pw presubmit --install

This will be effectively the same as running the following command before every
``git push``:

.. code-block:: bash

  $ pw presubmit


.. image:: ../pw_presubmit/docs/pw_presubmit_demo.gif
  :width: 800
  :alt: pw presubmit demo

If you ever need to bypass the presubmit hook (due to it being broken, for
example) you may push using this command:

.. code-block:: bash

  $ git push origin HEAD:refs/for/main --no-verify

Presubmit and branch management
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When creating new feature branches, make sure to specify the upstream branch to
track, e.g.

.. code-block:: bash

  $ git checkout -b myfeature origin/main

When tracking an upstream branch, ``pw presubmit`` will only run checks on the
modified files, rather than the entire repository.

Presubmit flags
^^^^^^^^^^^^^^^
``pw presubmit`` can accept a number of flags

``-b commit, --base commit``
  Git revision against which to diff for changed files. Default is the tracking
  branch of the current branch. Set commit to "HEAD" to check files added or
  modified but not yet commited. Cannot be used with --full.

``--full``
  Run presubmit on all files, not just changed files. Cannot be used with
  --base.

``-e regular_expression, --exclude regular_expression``
  Exclude paths matching any of these regular expressions, which are interpreted
  relative to each Git repository's root.

``-k, --keep-going``
  Continue instead of aborting when errors occur.

``--output-directory OUTPUT_DIRECTORY``
  Output directory (default: <repo root>/out/presubmit)

``--package-root PACKAGE_ROOT``
  Package root directory (default: <output directory>/packages)

``--clear, --clean``
  Delete the presubmit output directory and exit.

``-p, --program PROGRAM``
  Which presubmit program to run

``--step STEP``
  Provide explicit steps instead of running a predefined program.

``--install``
  Install the presubmit as a Git pre-push hook and exit.

.. _Sphinx: https://www.sphinx-doc.org/

.. inclusive-language: disable

.. _reStructuredText Primer: https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html

.. inclusive-language: enable

.. toctree::
  :maxdepth: 1

  embedded_cpp_guide
  style_guide
