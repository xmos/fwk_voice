####################
Documentation Source
####################

This folder contains source files for the **Voice Framework** documentation.  The sources do not render well in GitHub or an RST viewer. In addition, some information is not visible at all and some links will not be functional.

**********************
Building Documentation
**********************

=============
Prerequisites
=============

Install `Docker <https://www.docker.com/>`_.

Pull the docker container:

.. code-block:: console

    $ docker pull ghcr.io/xmos/doc_builder:main

========
Building
========

To build the documentation, run the following command in the root of the repository:

.. code-block:: console

    $ docker run --rm -t -u "$(id -u):$(id -g)" -v $(pwd):/build -e REPO:/build -e DOXYGEN_INCLUDE=/build/doc/Doxyfile.inc -e EXCLUDE_PATTERNS=/build/doc/exclude_patterns.inc -e DOXYGEN_INPUT=ignore ghcr.io/xmos/doc_builder:main

HTML document output is saved in the ``doc/_build/latest/html`` folder.  Open ``index.html`` to preview the saved documentation.

**********************
Adding a New Component
**********************

Follow the following steps to add a new component.

- Add an entry for the new component's top-level document to the appropriate TOC in the documents tree.
- If the new component uses `Doxygen`, append the appropriate path(s) to the INPUT variable in `Doxyfile.inc`.
- If the new component includes `.rst` files that should **not** be part of the documentation build, append the appropriate pattern(s) to `exclude_patterns.inc`.

***
FAQ
***

Q: Is it possible to build just a subset of the documentation?

A: Yes, however it is not recommended at this time.

Q: Is it possible to used the ``livehtml`` feature of Sphynx?

A: No, but ``livehtml`` support may be added to the XMOS ``doc_builder`` Docker container in the future.

Q: Where can I learn more about the XMOS ``doc_builder`` Docker container?

A: See the https://github.com/xmos/doc_builder repository.  See the ``doc_builder`` repository README for details on additional build options.

Q: How do I suggest enhancements to the XMOS ``doc_builder`` Docker container?

A: Create a new issue here: https://github.com/xmos/doc_builder/issues

Q: I don't need to run the link checking, can I disable that to make the build faster?

A: Yes, add ``-e SKIP_LINK=1`` to the ``docker run`` command line above.
