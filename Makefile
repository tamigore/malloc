# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: tamigore <tamigore@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/09/15 17:20:40 by tamigore          #+#    #+#              #
#    Updated: 2025/09/15 17:20:43 by tamigore         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

################################################################################
#                               Filename output                                #
################################################################################

ifeq ($(HOSTTYPE),)
HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

NAME        = libft_malloc_$(HOSTTYPE).so
SYMLINK     = libft_malloc.so

################################################################################
#                               Sources filenames                              #
################################################################################

SRCS_DIR    = sources
OBJS_DIR    = objects
DEPS_DIR    = $(OBJS_DIR)
C_FILES     = $(shell find $(SRCS_DIR) -name '*.c')
C_OBJS      = $(patsubst $(SRCS_DIR)/%.c,$(OBJS_DIR)/%.o,$(C_FILES))

################################################################################
#                              Commands and arguments                          #
################################################################################

CC          = gcc
CFLAGS      = -Iincludes -Wall -Wextra -Werror -g3 -fPIC -MMD -MP
RM          = rm -rf

################################################################################
#                         External Libraries                                   #
################################################################################

LIBFT       = libft/libft.a

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

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.c
	@ echo "\t$(_YELLOW) compiling... $*.c$(_NC)"
	@$(CC) $(CFLAGS) -c $< -o $@

################################################################################
#                                  Dependances                                 #
################################################################################

C_DEPS      = $(patsubst $(OBJS_DIR)/%.o,$(DEPS_DIR)/%.d,$(C_OBJS))
DEP_FILES   = $(C_DEPS)

$(shell mkdir -p $(sort $(dir $(DEP_FILES))))

-include $(DEP_FILES)

################################################################################
#                                   Command                                    #
################################################################################

all: libft $(NAME) symlink

libft:
	@ echo "$(_CYAN)[Compiling libft]$(_NC)"
	@ $(MAKE) -C libft

$(NAME): $(C_OBJS)
	@ echo "\t$(_YELLOW)[Creating shared library]$(_NC)"
	@$(CC) -shared -o $(NAME) $(C_OBJS) $(LIBFT)
	@ echo "$(_GREEN)[library created & ready]$(_NC)"

symlink: $(NAME)
	@ ln -sf $(NAME) $(SYMLINK)
	@ echo "$(_CYAN)[Symlink created: $(SYMLINK) -> $(NAME)]$(_NC)"

clean:
	@ echo "$(_RED)[cleaning up objects files]$(_NC)"
	@$(RM) $(OBJS_DIR)
	@$(RM) $(DEPS_DIR)
	@$(MAKE) -C libft clean

fclean: clean
	@ echo "$(_RED)[cleaning up library files]$(_NC)"
	@$(RM) $(NAME) $(SYMLINK)
	@$(MAKE) -C libft fclean

re: fclean all

.PHONY: all libft clean fclean re symlink