/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/15 19:03:29 by fandre-m          #+#    #+#             */
/*   Updated: 2026/09/15 19:03:29 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	main(int ac, char **av)
{
	if (ac != 8)
		return (error_msg("Wrong number of arguments."));
	char	*num_1;
	num_1 = av[1];
	printf("%s", num_1);
	return (0);
}
