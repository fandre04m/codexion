# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/09/15 16:46:10 by fandre-m          #+#    #+#              #
#    Updated: 2026/10/27 16:24:04 by fandre-m         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = codexion

SRC = main.c utils.c parsing.c init.c safe_mutex.c safe_cond.c safe_thread.c

OBJ = $(SRC:.c=.o)

RM = rm -rf

CC = cc

CFLAGS = -Wall -Wextra -Werror -pthread

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: al clean fclean re
