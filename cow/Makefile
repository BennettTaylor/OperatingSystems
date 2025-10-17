TESTS = test_clone_large \
        test_clone_read \
        test_clone \
        test_create \
        test_edge_cases \
        test_many_cow \
        test_one_cow \
        test_read \
        test_write_read \
        test_write

CFLAGS := -std=gnu99 -O0 -g -pthread $(CFLAGS)
LDLIBS := -pthread $(LDLIBS)

.PHONY: all

all: $(TESTS)

tls.o: tls.c tls.h
	$(CC) $(CFLAGS) -c $< -o $@

%: %.c tls.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

.PHONY: clean
clean:
	rm -f tls.o $(TESTS)
	rm -f *~