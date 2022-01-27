# darlingserver Duct Taping

The purpose of this subproject is to use a significantly stripped down version of XNU's own kernel code coupled with some glue/duct-tape code to simulate the necessary parts of a normal XNU kernel environment for the code to work properly to implement things like Mach IPC.

## Client-Side `kqueue` Implementation

This subproject also contains the client-side kqueue implementation that libsystem_kernel uses.
It is based on the XNU kqueue code with some additional duct-taping code to allow us to closely replicate XNU's semantics.
