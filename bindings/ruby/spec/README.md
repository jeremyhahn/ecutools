# J2534 Integration Tests

Tests are in their own file so libj2534.so gets loaded fresh with each test to ensure isolation. Otherwise, all tests will share the same libj2534 state.

> Running these integration tests may incur AWS fees.

	rspec spec/name_of_integration_spec.rb

