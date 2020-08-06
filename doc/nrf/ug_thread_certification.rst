.. _ug_thread_cert:

Thread certification
####################

Certification is required by Thread Group for devices using the Thread protocol. This page describes the particularities of Nordic Semiconductor devices
using nRF Connect SDK when comes to Thread certification.

For general details about the certification process, check the `Thread Group's certification information`_.

.. contents::
    :local:
    :depth: 2

Thread certification options
****************************

Depending on your development approach, you have the following certification options when using Nordic Semiconductor devices.

Certification by inheritance without modifications to binaries
==============================================================

When developing a Thread product with nRF Connect SDK, you can apply for certifying this product by inheritance as an official Thread certified product.
This is possible for products that are based on systems from Nordic (both stack and device) that were previously certified by Thread Group,
as long as you have not modified the initial functionality that was tested by Thread Group for certification.

This scenario offers the most simplified, potentially shorter certification process at the lowest cost.
It is available to `Thread Group members`_ at Implementer tier or higher.

Check :ref:`nrfxlib:ot_libs` for the latest available OpenThread certified libraries with nRF Connect SDK and their usage.

Certification by inheritance with modifications to binaries
===========================================================

If your solution uses the same version of the OpenThread stack as the one used in the precompiled, certified binaries,
but you made modifications within the operating parameters defined by Thread Group,
you need to contact Thread Group to check if certification by inheritance is still possible.
You have to officially list all changes with detailed explanation in your application.
Based on this list, Thread Group can accept or reject your application for certification by inheritance.

Certification at ATL
====================

If your solution modifies the stack's certified functionality or if Thread Group rejects your application for certification by inheritance,
you need to undergo the full certification procedure at an ATL.
Only Contributor and Sponsor tier members of Thread Group can apply for certification by an ATL.

Thread Test Harness integration
*******************************

To certify a Thread product, all certification test cases must be run on the Thread Test Harness software.

A detailed description on how to assemble and configure Thread Test Bed and run Thread Test Harness can be found on the `Thread Group's Confluence wiki page`_.

The following procedure references the nRF52840 Development Kit. The analogical procedure may be used to run the certification using other development kits.

Complete the following steps to run the certification tests:


Build the certification image
=============================

The 'openthread/cli' sample is used as a base, modifiying it with an overlay file.

::

 cd ncs/nrf/samples/openthread/cli/
 west build -b nrf52840dk_nrf52840 -- -DCONF_FILE="prj.conf harness/overlay-cert.conf"

 Notice that the overlay file selects by defaul the precompiled OpenThread libraries.

Prepare Thread Test Harness
===========================

Copy the provided ``ncs/nrf/samples/openthread/cli/harness/nrf52840-ncs.py`` file into ``C:\GRL\Thread1.1\Thread_Harness\THCI``.

Copy the provided ``ncs/nrf/samples/openthread/cli/harness/nrf52840.jpg`` file into ``C:\GRL\Thread1.1\Web\images``.

Edit ``C:\GRL\Thread1.1\Web\data\deviceInputFields.xml`` and prepend this:

::

    <DEVICE name="nRF52840-ncs" thumbnail="nRF52840.jpg" description = "Nordic Semiconductor: nRF52840 (NCS) Baudrate:115200" THCI="nRF52840-ncs">
        <ITEM label="Serial Line"
              type="text"
              forParam="SerialPort"
              validation="COM"
              hint="eg: COM1">COM
        </ITEM>
        <ITEM label="Speed"
              type="text"
              forParam="SerialBaudRate"
              validation="baud-rate"
              hint="eg: 115200">115200
        </ITEM>
    </DEVICE>
```

Note for Thread Test Harness software
=====================================

The procedure of running the Thread Test Harness software on Nordic Semiconductor's nRF52840 SoC (on the PCA10056 Development Kit) must be adjusted in the following way.

Thread Test Harness does not correctly identify the PCA10056 Development Kit out-of-the-box.
Due to a collision of USB PID:VID with another vendor (this is valid only for Nordic Semiconductor development kits with J-Link virtual COM port),
Nordic devices are not automatically added to the device list.

To add an nRF52840 device, drag the nRF52840 device and drop it on the configuration page. After that, the devices are configured and proper baud rate (115200) and COM port is set.

Useful links
============

See the following links for more information:

- `OpenThread THCI`_
- `Openthread acting as a new reference platform`_
