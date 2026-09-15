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

NAME = codexion.a

SRC = test.c

OBJ = $(SRC:.c=.o)

RM = rm -rf

CFLAGS = -Wall -Wextra -Werror

AR = ar rcs

all: $(NAME)

$(NAME): $(OBJ)
	$(AR) $(NAME) $(OBJ)

%.o: %.c
	cc $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: al clean fclean re
