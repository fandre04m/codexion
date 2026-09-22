/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parsing.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/21 16:29:15 by fandre-m          #+#    #+#             */
/*   Updated: 2026/09/21 16:29:15 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	parse_input(t_sim *sim, char **av)
{
	int	i;
	int	num_len;

	i = 1;
	while (i <= 8)
	{
		num_len = strlen(av[i]);
		if (num_len > 10)
		{
			error_msg("One or more arguments is bigger than INT_MAX.");
		}
		else
		{
			sim->n_coders = 5;
		}
		i++;
	}
}
