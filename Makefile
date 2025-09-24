# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/09/15 17:20:40 by tamigore          #+#    #+#              #
#    Updated: 2025/09/24 16:41:13 by tamigore         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

################################################################################
#                               Filename output                                #
################################################################################

ifeq ($(HOSTTYPE),)
HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

.DEFAULT_GOAL := all

NAME         = libft_malloc_$(HOSTTYPE).so
SYMLINK      = libft_malloc.so
TEST         = test.out
TEST_CUSTOM  = test_custom.out
BENCH        = bench.out
BENCH_CUSTOM = bench_custom.out

################################################################################
#                               Sources filenames                              #
################################################################################

SRCS_DIR    = sources
OBJS_DIR    = objects
DEPS_DIR    = $(OBJS_DIR)
C_FILES     = $(shell find $(SRCS_DIR) -name '*.c')
C_OBJS      = $(patsubst $(SRCS_DIR)/%.c,$(OBJS_DIR)/%.o,$(C_FILES))

TEST_DIR   	= tests
TEST_SRC = $(TEST_DIR)/test.c $(TEST_DIR)/custom_tests.c
BENCH_FILE  = $(TEST_DIR)/bench.c
BENCH_OBJ   = $(patsubst $(TEST_DIR)/%.c,$(OBJS_DIR)/%.o,$(BENCH_FILE))

################################################################################
#                              Commands and arguments                          #
################################################################################

CC          = gcc

LIBRARY_FLAGS		= -fPIC
INCLUDES_FLAGS		= -Iincludes
ERROR_FLAGS			= -Wall -Wextra -Werror -g3
DEPENDENCY_FLAGS	= -MMD -MP
THREADS_FLAGS		= -lpthread
MALLOC_FLAGS		= -Wl,-rpath,. -L. -lft_malloc
TEST_FLAGS			= -lcunit
CUSTOM_DEFINE		= -DCUSTOM_ALLOCATOR

CFLAGS     	+=	$(INCLUDES_FLAGS)	\
				$(DEPENDENCY_FLAGS)	\
				$(ERROR_FLAGS)		\
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

TEST_DEP	 = $(patsubst %.out,%.d,$(TEST))
BENCH_DEP	 = $(patsubst %.out,%.d,$(BENCH))

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

fclean: clean
	@ echo "$(_RED)[cleaning up library files]$(_NC)"
	@$(RM) $(NAME) $(SYMLINK)
	@$(RM) $(TEST) $(TEST_CUSTOM) $(BENCH) $(BENCH_CUSTOM)

test: symlink $(TEST_SRC)
	@ echo "\t$(_CYAN)[Building baseline test]$(_NC)"
	@ $(CC) -o $(TEST) $(TEST_SRC) $(CFLAGS) $(THREADS_FLAGS) $(TEST_FLAGS)
	@ echo "\t$(_CYAN)[Building custom test]$(_NC)"
	@ $(CC) -o $(TEST_CUSTOM) $(TEST_SRC) $(CFLAGS) $(MALLOC_FLAGS) $(THREADS_FLAGS) $(TEST_FLAGS)
	@ echo "\t$(_GREEN)[Test binaries built: $(TEST), $(TEST_CUSTOM)]$(_NC)"

$(BENCH_OBJ): $(BENCH_FILE)
	@ echo "\t$(_YELLOW) compiling... $*.c$(_NC)"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

bench: symlink $(BENCH_OBJ)
	@ echo "$(_CYAN)[Building benchmark binary]$(_NC)"
	@ $(CC) -o $(BENCH) $(BENCH_OBJ) $(CFLAGS)
	@ echo "$(_CYAN)[Building benchmark (linked with allocator)]$(_NC)"
	@ $(CC) -o $(BENCH_CUSTOM) $(BENCH_OBJ) $(CFLAGS) $(MALLOC_FLAGS)
	@ echo "$(_GREEN)[Benchmark binary built: $(BENCH)]$(_NC)"

perf: all bench
	@ echo "$(_CYAN)[Performance comparaison]$(_NC)"
	@ echo "$(_YELLOW)Baseline (libc) run:$(_NC)"
	@ /usr/bin/time -f 'real %E user %U sys %S' ./$(BENCH)
	@ echo "$(_YELLOW)Custom allocator run (LD_PRELOAD)::$(_NC)"
	@ /usr/bin/time -f 'real %E user %U sys %S' ./$(BENCH_CUSTOM)
	@ echo "$(_CYAN)[Done]$(_NC)"

sanitize: all test
	@ echo "$(_CYAN)[AddressSanitizer]$(_NC)"
	valgrind ./$(TEST)
	@ echo "$(_CYAN)[Done]$(_NC)"

re: fclean all

.PHONY: all clean fclean re symlink test perf sanitize bench