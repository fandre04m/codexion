/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/15 19:25:06 by fandre-m          #+#    #+#             */
/*   Updated: 2026/09/15 19:25:06 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <stdio.h>
# include <stdbool.h>
# include <unistd.h>
# include <string.h>

/*
 *  ASNI Escape sequence for bold colored text.
 *  usage:
 *		printf(R"This is red text."RST);
 *	Use RST to reset color back to default.
 */
# define RST	"\033[0m"		/* Reset to default color */
# define R		"\033[1;31m"	/* Bold red */
# define G		"\033[1;32m"	/* Bold green */
# define W		"\033[1;37m"	/* Bold white */

/*
 * Required imput format:
 *					'5 800 100 100 100 5 50 fifo'
 *	n_coders/burnout/compile/debug/refact/n_compiles/cooldown/scheduler
 *	all these are times in miliseconds, last must be str 'fifo' or 'edf'
 */

/* ------- Structs --------*/
typedef struct s_dongle
{
	bool	in_cooldown;
	int		id;
}	t_dongle;

typedef struct s_coder
{
	int		id;
	int		n_compiles;
	bool	all_compiles;
	int		last_comp_time;
	// t_dongle	*left_dongle;
	// t_dongle	*right_dongle;
	// pthread_t	*thread_id;
}	t_coder;

typedef struct s_sim
{
	int			n_coders;
	int			time_burnout;
	int			time_compile;
	int			time_debug;
	int			time_refact;
	int			req_compiles;
	int			dongle_cooldown;
	char		*scheduler;
	int			start_sim_time;
	bool		end_sim;
	// t_dongle	*dongles;
	// t_coder		*coders;
}	t_sim;

/* -------- Utils ---------*/
int		error_msg(const char *error);

/* ------- Parsing --------*/
int	parse_input(t_sim *sim, char **av);

#endif
