.. _udho_build:

Building udho
=============

udho can be built like a regular cmake library. The requirements of udho library are available in most *nix reposetories.

Requirements
------------

* boost >= 1.66
* icu [optional]
* git
* cmake
* c++ compiler

Ubuntu
^^^^^^

.. code-block:: bash

   sudo apt-get install git cmake build-essential boost-all-dev libicu-dev pugixml-dev

Fedora
^^^^^^

.. code-block:: bash

   sudo dnf install @development-tools
   sudo dnf install git cmake boost-devel libicu-devel pugixml-devel

Arch Linux
^^^^^^^^^^

.. code-block:: bash

   sudo pacman -S base-devel git cmake boost icu pugixml

Mac
^^^

.. code-block:: bash

   brew install git cmake boost icu pugixml

Build
-----

After the dependencies are installed udho has to be cloned from the git reposetory and ompiled as a regular C++ library. use ``make tests`` to execute the unit tests. The exemaples are inside ``examples`` directory.

.. code-block:: bash

   git clone https://gitlab.com/neel.basu/udho.git
   cd udho
   mkdir build
   cd build
   cmake ..
   make
