CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99

# Source files
SERVER_SRC = server.c utils.c persistence.c
PUBLISHER_SRC = publisher.c
SUBSCRIBER_SRC = subscriber.c

# Object files
SERVER_OBJ = $(SERVER_SRC:.c=.o)
PUBLISHER_OBJ = $(PUBLISHER_SRC:.c=.o)
SUBSCRIBER_OBJ = $(SUBSCRIBER_SRC:.c=.o)

# Executables
SERVER_EXEC = server
PUBLISHER_EXEC = publisher
SUBSCRIBER_EXEC = subscriber

# Test files
TEST_SRCS = tests/test_runner.c tests/test_utils.c tests/test_message_parsing.c tests/test_persistence.c
TEST_OBJS = $(TEST_SRCS:.c=.o) utils.o persistence.o
TEST_EXEC = test_runner

# Coverage specific flags
COVERAGE_CFLAGS = $(CFLAGS) -fprofile-arcs -ftest-coverage
COVERAGE_LDFLAGS = -fprofile-arcs -ftest-coverage

.PHONY: all clean test lint coverage docs help

all: $(SERVER_EXEC) $(PUBLISHER_EXEC) $(SUBSCRIBER_EXEC)

$(SERVER_EXEC): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(PUBLISHER_EXEC): $(PUBLISHER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(SUBSCRIBER_EXEC): $(SUBSCRIBER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Rule for compiling with coverage flags
coverage_%.o: %.c
	$(CC) $(COVERAGE_CFLAGS) -c -o $@ $<

test: $(TEST_EXEC)
	./$(TEST_EXEC)

$(TEST_EXEC): $(TEST_OBJS)
	$(CC) $(CFLAGS) -Wl,--wrap,write -o $@ $^

lint:
	@echo "Generating compile_commands.json with bear..."
	bear -- $(MAKE) clean all
	@echo "Running clang-tidy..."
	clang-tidy -p . --checks='-*,clang-analyzer-*' $(SERVER_SRC) $(PUBLISHER_SRC) $(SUBSCRIBER_SRC) tests/*.c

coverage:
	@echo "Building with coverage flags..."
	$(MAKE) clean
	$(MAKE) CFLAGS="$(COVERAGE_CFLAGS)" LDFLAGS="$(COVERAGE_LDFLAGS)" $(TEST_EXEC)
	@echo "Running tests for coverage..."
	./$(TEST_EXEC)
	@echo "Generating coverage report..."
	@for file in $(SERVER_SRC) $(PUBLISHER_SRC) $(SUBSCRIBER_SRC) persistence.c utils.c; do \
		gcov $$file; \
	done
	@echo "Coverage report generated. Look for .gcov files."

docs:
	@echo "Generating Doxygen documentation..."
	doxygen Doxyfile
	@echo "Doxygen documentation generated in html/."

help:
	@echo "Usage: make <command>\n"
	@echo "Commands:\n"
	@echo "  all       Builds all executables (server, publisher, subscriber)."
	@echo "  clean     Removes all built files and temporary artifacts."
	@echo "  test      Runs all unit tests."
	@echo "  lint      Runs clang-tidy for static analysis and linting (requires bear and clang-tidy)."
	@echo "  coverage  Generates code coverage reports (requires gcov)."
	@echo "  docs      Generates Doxygen HTML documentation (requires doxygen)."
	@echo "  help      Displays this help message."

clean:
	rm -f $(SERVER_EXEC) $(PUBLISHER_EXEC) $(SUBSCRIBER_EXEC) $(TEST_EXEC) *.o tests/*.o *.gcda *.gcno *.gcov compile_commands.json
	rm -rf html