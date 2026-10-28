/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/15 19:25:06 by fandre-m          #+#    #+#             */
/*   Updated: 2026/10/28 20:36:50 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <stdbool.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <errno.h>

/*
 *  ASNI Escape sequence for bold colored text.
 *  usage:
 *		printf(R"This is red text."RST);
 *	Use RST to reset color back to default.
 */
# define RST "\033[0m"  /* Reset to default color */
# define R "\033[1;31m" /* Bold red */
# define G "\033[1;32m" /* Bold green */
# define W "\033[1;37m" /* Bold white */

// type definitons for convenience.
typedef pthread_mutex_t	t_mtx;
typedef pthread_cond_t	t_cond;
typedef struct s_sim	t_sim;

/*--------- Enums ---------*/

typedef enum e_mutex_op
{
	M_INIT,
	M_LOCK,
	M_UNLOCK,
	M_DESTROY
}	t_mutex_op;

typedef enum e_cond_op
{
	C_INIT,
	C_DESTROY,
	C_WAIT,
	C_TIMEDWAIT,
	C_SIGNAL,
	C_BROADCAST
}	t_cond_op;

/*-------- Structs --------*/
typedef struct s_dongle
{
	t_mtx	dongle;
	int		dongle_id;
	bool	in_use;
	long	ready_at;
}	t_dongle;

typedef struct s_coder
{
	int			coder_id;
	int			n_compiles;
	bool		finished;
	int			last_comp_time;
	t_dongle	*left_dongle;
	t_dongle	*right_dongle;
	pthread_t	thread_id;
	t_sim		*sim;
}	t_coder;

struct s_sim
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
	t_dongle	*dongles;
	t_coder		*coders;
};

/*--------- Utils ---------*/
int	error_msg(const char *error);

/*-------- Parsing --------*/
int	parse_input(t_sim *sim, char **av);

/*--------- Init ----------*/
int	init_data(t_sim *sim);

/*------- Handlers --------*/
int	safe_mutex_handle(t_mtx *mutex, t_mutex_op op);

#endif
