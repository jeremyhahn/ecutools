# J2534 Integration Tests

Tests that are sensitive to state have their own file due to the inability to unload a Shared Object file (libj2534.so).

This way, senitive tests can be run in isolation without state from prior tests interfering.

