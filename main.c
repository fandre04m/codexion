/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/15 19:03:29 by fandre-m          #+#    #+#             */
/*   Updated: 2026/10/29 01:55:40 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/*
 * Required imput format:
 *					'5 800 100 100 100 5 50 fifo'
 *	n_coders/burnout/compile/debug/refact/n_compiles/cooldown/scheduler
 *	all these are times in miliseconds, last must be str 'fifo' or 'edf'
 */

int	main(int ac, char **av)
{
	t_sim		simulation;

	memset(&simulation, 0, sizeof(simulation));
	if (ac == 9)
	{
		if (parse_input(&simulation, av))
			return (1);
		if	(init_data(&simulation))
		{
			clean_sim(&simulation);
			return (1);
		}
		// 3-Start simulation
		// simulation_start(&simulation); TODO
		if (clean_sim(&simulation))
			return (1);
	}
	else
	{
		return (error_msg("Wrong number of arguments.\n"
				G"Correct example: '5 800 100 100 100 5 50 fifo'"RST, 1));
	}
	return (0);
}
