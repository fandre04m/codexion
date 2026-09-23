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

/*
 * Required imput format:
 *					'5 800 100 100 100 5 50 fifo'
 *	n_coders/burnout/compile/debug/refact/n_compiles/cooldown/scheduler
 *	all these are times in miliseconds, last must be str 'fifo' or 'edf'
 */

int	main(int ac, char **av)
{
	t_sim	simulation;

	if (ac == 9)
	{
		if (parse_input(&simulation, av))
			return (1);
		//
		// 2-create all instances (malloc)
		// data_init(&simulation); TODO
		//
		// 3-Start simulation
		// simulation_start(&simulation); TODO
		//
		// 4-Clean all data (No leaks)
		// clean_sim(&simulation); TODO
	}
	else
	{
		return (error_msg("Wrong number of arguments.\n"
				G"Correct example: '5 800 100 100 100 5 50 fifo'"RST));
	}
	return (0);
}
