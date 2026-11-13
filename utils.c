/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/18 17:07:38 by fandre-m          #+#    #+#             */
/*   Updated: 2026/11/12 00:55:04 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long long	current_time_ms(void)
{
	struct timespec	curr_time;
	long long		now;

	if (clock_gettime(CLOCK_MONOTONIC, &curr_time))
		return (error_msg("Clock failed to get current time."));
	now = (curr_time.tv_sec * 1000) + (curr_time.tv_nsec / 1000000);
	return (now);
}

int	error_msg(const char *msg)
{
	fprintf(stderr, R "Error: %s\n" RST, msg);
	return (1);
}
