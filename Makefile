# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/09/15 16:46:10 by fandre-m          #+#    #+#              #
#    Updated: 2026/09/15 19:28:34 by fandre-m         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = codexion

SRC = main.c utils.c parsing.c init.c

OBJ = $(SRC:.c=.o)

RM = rm -rf

CC = cc

CFLAGS = -Wall -Wextra -Werror

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
