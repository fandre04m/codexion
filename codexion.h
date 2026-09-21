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
#define CODEXION_H

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

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
 *	burnout - compile - debug - refact - n_compiles - cooldown - scheduler
 *	all these are times in miliseconds, last must be str 'fifo' or 'edf'
*/

/* -------- Utils ---------*/
int	error_msg(const char *error);

#endif
