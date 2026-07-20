# Build all firmware in the repo, one project directory at a time.
build:
	@ci/build.sh

.PHONY: build

# Remove all built firmware in the repo, one project directory at a time.
clean:
	@ci/clean.sh

.PHONY: clean

# Format all sources in place.
format:
	@ci/format.sh

# Check that all sources are formatted, without modifying them.
format-check:
	@ci/format.sh --check
