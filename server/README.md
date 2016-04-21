###### Open Source ECU Tuning and Diagnostics

The ecutools server acts as a centralized hub for real-time communications between pass-through devices and client web browsers.

The server is based on Maven and Eclipse Jetty.

### Build Dependencies

1. Maven 3.0

### Build

	mvn package

#### Project Dependencies

To following command will download all project dependencies to target/dependency:

	mvn dependency:copy-dependencies

### Run

To following command will start the ecutools.io server in development mode:

	mvn jetty:run
