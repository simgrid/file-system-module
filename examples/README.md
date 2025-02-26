# FSMod Examples

The `./examples/` directory contains code for example programs that use FSMod, as described below. Building these examples is done simply via `make examples`.

## JBOD (Just a Bunch of Disks) example

This program (`./examples/jbod.cpp`) demonstrates the use of JBOD storage with a simple client-server setup, where the client (the `FileWriteActor`) does a file write operation on a JBOD storage located at a remote host.  The JBOD setup, which happens to comprise four distinct disks, is completely transparent to the client.

## Open-Seek-Write example

This program (`./examples/open_seek_write.cpp`) demonstrates the use of the FSMod API to open a file, seek to a particular offset, and write data at that offset, before closing the file. This is for a one-host setup with a single local disk. Two actors are creates on that host and each performs a write operation (at slightly different times, so that they overlap in time). 
 
