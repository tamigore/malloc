# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/09/15 17:20:40 by tamigore          #+#    #+#              #
#    Updated: 2025/09/24 19:48:00 by automated        ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

################################################################################
#                               Filename output                                #
################################################################################

ifeq ($(HOSTTYPE),)
HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

.DEFAULT_GOAL := all

NAME               = libft_malloc_$(HOSTTYPE).so
SYMLINK            = libft_malloc.so
TEST               = test.out
TEST_CUSTOM        = test_custom.out
BENCH              = bench.out
BENCH_CUSTOM       = bench_custom.out
MICRO_BENCH        = bench_micro.out
MICRO_BENCH_CUSTOM = bench_micro_custom.out

################################################################################
#                               Sources filenames                              #
################################################################################

SRCS_DIR    = sources
OBJS_DIR    = objects
DEPS_DIR    = $(OBJS_DIR)
C_FILES     = $(shell find $(SRCS_DIR) -name '*.c')
C_OBJS      = $(patsubst $(SRCS_DIR)/%.c,$(OBJS_DIR)/%.o,$(C_FILES))

TEST_DIR   	= tests
TEST_MAIN_SRC = $(TEST_DIR)/test.c
CUSTOM_TEST_SRC = $(TEST_DIR)/custom_tests.c
TEST_MAIN_OBJ = $(OBJS_DIR)/test.o
CUSTOM_TEST_OBJ = $(OBJS_DIR)/custom_tests.o
BENCH_FILE  = $(TEST_DIR)/bench.c
BENCH_OBJ   = $(patsubst $(TEST_DIR)/%.c,$(OBJS_DIR)/%.o,$(BENCH_FILE))

################################################################################
#                              Commands and arguments                          #
################################################################################

CC          = gcc

LIBRARY_FLAGS		= -fPIC
INCLUDES_FLAGS		= -Iincludes
ERROR_FLAGS			= $(BASE_WARN) $(DEBUG_FLAGS)
DEPENDENCY_FLAGS	= -MMD -MP
THREADS_FLAGS		= -lpthread
MALLOC_FLAGS		= -Wl,-rpath,. -L. -lft_malloc
TEST_FLAGS			= -lcunit
CUSTOM_DEFINE		= -DCUSTOM_ALLOCATOR

# Build mode selection (default: debug)
# Usage: make perf MODE=release
BASE_WARN			= -Wall -Wextra -Werror
ifeq ($(MODE),release)
OPT_FLAGS			= -O3 -DNDEBUG
DEBUG_FLAGS			=
else
OPT_FLAGS			= -O0
DEBUG_FLAGS			= -g3
endif

CFLAGS     	+=	$(INCLUDES_FLAGS)	\
				$(DEPENDENCY_FLAGS)	\
				$(ERROR_FLAGS)		\
				$(OPT_FLAGS)		\
				$(LIBRARY_FLAGS)	\
				$(THREADS_FLAGS)

RM          = rm -rf

################################################################################
#                                 Defining colors                              #
################################################################################

_RED        = \033[31m
_GREEN      = \033[32m
_YELLOW     = \033[33m
_CYAN       = \033[96m
_NC         = \033[0m

################################################################################
#                                    Objects                                   #
################################################################################

$(shell mkdir -p $(sort $(dir $(C_OBJS))))

################################################################################
#                                  Dependances                                 #
################################################################################

C_DEPS      = $(patsubst $(OBJS_DIR)/%.o,$(DEPS_DIR)/%.d,$(C_OBJS))
DEP_FILES   = $(C_DEPS)

$(shell mkdir -p $(sort $(dir $(DEP_FILES))))

TEST_DEP	 = $(patsubst %.out,%.d,$(TEST)) \
			   $(patsubst %.out,%.d,$(TEST_CUSTOM))

-include $(DEP_FILES)

################################################################################
#                                   Command                                    #
################################################################################

all: $(SYMLINK)

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.c
	@ echo "\t$(_YELLOW) compiling... $*.c$(_NC)"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(C_OBJS)
	@ echo "\t$(_YELLOW)[Creating shared library]$(_NC)"
	@$(CC) -shared -o $(NAME) $(C_OBJS) $(LIBFT) $(THREADS_FLAGS)
	@ echo "$(_GREEN)[library created & ready]$(_NC)"

$(SYMLINK): $(NAME)
	@ ln -sf $(NAME) $(SYMLINK)
	@ echo "$(_CYAN)[Symlink created: $(SYMLINK) -> $(NAME)]$(_NC)"

clean:
	@ echo "$(_RED)[cleaning up objects files]$(_NC)"
	@$(RM) $(OBJS_DIR)
	@$(RM) $(DEPS_DIR)
	@$(RM) $(TEST_DEP)

fclean: clean
	@ echo "$(_RED)[cleaning up library files]$(_NC)"
	@$(RM) $(NAME) $(SYMLINK)
	@$(RM) $(TEST) $(TEST_CUSTOM) $(BENCH) $(BENCH_CUSTOM) $(MICRO_BENCH) $(MICRO_BENCH_CUSTOM)

$(TEST_MAIN_OBJ): $(TEST_MAIN_SRC)
	@ echo "\t$(_YELLOW) compiling test main... test.c$(_NC)"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(CUSTOM_TEST_OBJ): $(CUSTOM_TEST_SRC)
	@ echo "\t$(_YELLOW) compiling custom tests (CUSTOM_ALLOCATOR)... custom_tests.c$(_NC)"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -DCUSTOM_ALLOCATOR -c $< -o $@

$(TEST): $(TEST_MAIN_OBJ)
	@ echo "\t$(_CYAN)[Link] baseline test$(_NC)"
	@ $(CC) -o $@ $^ $(CFLAGS) $(THREADS_FLAGS) -lcunit

$(TEST_CUSTOM): $(SYMLINK) $(TEST_MAIN_OBJ) $(CUSTOM_TEST_OBJ)
	@ echo "\t$(_CYAN)[Link] custom test$(_NC)"
	@ $(CC) -o $@ $(TEST_MAIN_OBJ) $(CUSTOM_TEST_OBJ) $(CFLAGS) $(MALLOC_FLAGS) $(THREADS_FLAGS) -lcunit

test: $(TEST) $(TEST_CUSTOM)

$(BENCH_OBJ): $(BENCH_FILE)
	@ echo "\t$(_YELLOW) compiling... $*.c$(_NC)"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BENCH): $(BENCH_OBJ)
	@ echo "\t$(_CYAN)[Link] benchmark baseline$(_NC)"
	@ $(CC) -o $@ $^ $(CFLAGS) $(THREADS_FLAGS)

$(BENCH_CUSTOM): $(SYMLINK) $(BENCH_OBJ)
	@ echo "\t$(_CYAN)[Link] benchmark custom$(_NC)"
	@ $(CC) -o $@ $(BENCH_OBJ) $(CFLAGS) $(MALLOC_FLAGS) $(THREADS_FLAGS)

bench: $(BENCH) $(BENCH_CUSTOM)

$(OBJS_DIR)/bench_micro.o: $(TEST_DIR)/bench_micro.c
	@ echo "\t$(_YELLOW) compiling... bench_micro.c$(_NC)"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(MICRO_BENCH): $(OBJS_DIR)/bench_micro.o
	@ echo "\t$(_CYAN)[Link] micro benchmark baseline$(_NC)"
	@ $(CC) -o $@ $^ $(CFLAGS) $(THREADS_FLAGS)

$(MICRO_BENCH_CUSTOM): $(SYMLINK) $(OBJS_DIR)/bench_micro.o
	@ echo "\t$(_CYAN)[Link] micro benchmark custom$(_NC)"
	@ $(CC) -o $@ $(OBJS_DIR)/bench_micro.o $(CFLAGS) $(MALLOC_FLAGS) $(THREADS_FLAGS)

micro: $(MICRO_BENCH) $(MICRO_BENCH_CUSTOM)

# NOTE:
# Previous implementation used phony targets `all` and `bench` as prerequisites.
# Because they are phony, `make perf` always triggered relinking of benchmarks
# even when nothing changed. Now we depend directly on the actual file targets
# (shared library symlink + the two benchmark binaries) so a repeated
# `make perf` becomes a pure execution step with no needless relink.
perf: $(SYMLINK) $(BENCH) $(BENCH_CUSTOM)
	@ echo "$(_CYAN)[Performance comparaison]$(_NC)"
	@ echo "$(_YELLOW)Baseline (libc) run:$(_NC)"
	@ /usr/bin/time -f 'real %E user %U sys %S' ./$(BENCH)
	@ echo "$(_YELLOW)Custom allocator run (LD_PRELOAD)::$(_NC)"
	@ /usr/bin/time -f 'real %E user %U sys %S' ./$(BENCH_CUSTOM)
	@ echo "$(_CYAN)[Done]$(_NC)"

perf-micro: $(SYMLINK) $(MICRO_BENCH) $(MICRO_BENCH_CUSTOM)
	@ echo "$(_CYAN)[Performance micro scenarios]$(_NC)"
	@ echo "$(_YELLOW)fixed 100000 64$(_NC)"
	@ /usr/bin/time -f 'libc   real %E user %U sys %S' ./$(MICRO_BENCH) fixed 100000 64
	@ /usr/bin/time -f 'custom real %E user %U sys %S' ./$(MICRO_BENCH_CUSTOM) fixed 100000 64
	@ echo ""
	@ echo "$(_YELLOW)rand 100000 256$(_NC)"
	@ /usr/bin/time -f 'libc   real %E user %U sys %S' ./$(MICRO_BENCH) rand 100000 256
	@ /usr/bin/time -f 'custom real %E user %U sys %S' ./$(MICRO_BENCH_CUSTOM) rand 100000 256
	@ echo ""
	@ echo "$(_YELLOW)working set 200000 512 512$(_NC)"
	@ /usr/bin/time -f 'libc   real %E user %U sys %S' ./$(MICRO_BENCH) ws 200000 512 512
	@ /usr/bin/time -f 'custom real %E user %U sys %S' ./$(MICRO_BENCH_CUSTOM) ws 200000 512 512
	@ echo ""
	@ echo "$(_YELLOW)realloc mix 50000 32 4096$(_NC)"
	@ /usr/bin/time -f 'libc   real %E user %U sys %S' ./$(MICRO_BENCH) realloc 50000 32 4096
	@ /usr/bin/time -f 'custom real %E user %U sys %S' ./$(MICRO_BENCH_CUSTOM) realloc 50000 32 4096
	@ echo ""
	@ echo "$(_YELLOW)fragment 20000 128$(_NC)"
	@ /usr/bin/time -f 'libc   real %E user %U sys %S' ./$(MICRO_BENCH) frag 20000 128
	@ /usr/bin/time -f 'custom real %E user %U sys %S' ./$(MICRO_BENCH_CUSTOM) frag 20000 128
	@ echo "$(_CYAN)[Done micro]$(_NC)"

sanitize: all test
	@ echo "$(_CYAN)[AddressSanitizer]$(_NC)"
	valgrind ./$(TEST)
	@ echo "$(_CYAN)[Done]$(_NC)"

re: fclean all

.PHONY: all clean fclean re symlink test perf sanitize bench micro perf-micro