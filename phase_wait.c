/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   phase_wait.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/18 12:22:10 by fandre-m          #+#    #+#             */
/*   Updated: 2026/11/18 12:22:10 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	get_deadline(long long remaining_ms, struct timespec *realtime)
{
	long long	seconds;
	long long	millisecs;
	long long	nano_secs;

	seconds = remaining_ms / 1000;
	millisecs = remaining_ms % 1000;
	nano_secs = millisecs * 1000000;
	realtime->tv_sec += seconds;
	if (realtime->tv_nsec >= 1000000000 - nano_secs)
	{
		realtime->tv_nsec -= 1000000000 - nano_secs;
		realtime->tv_sec++;
	}
	else
		realtime->tv_nsec += nano_secs;
}

bool	request_wait(t_sim *sim, long long to_wait)
{
	bool			phase_finished;
	long long		phase_end;
	long long		remaining_ms;
	struct timespec	realtime;

	phase_finished = false;
	phase_end = current_time_ms() + to_wait;
	safe_mutex_handle(&sim->sim_mtx, M_LOCK);
	while (!sim->end_sim && phase_end > current_time_ms())
	{
		remaining_ms = phase_end - current_time_ms();
		if (remaining_ms <= 0)
			break;
		clock_gettime(CLOCK_REALTIME, &realtime);
		get_deadline(remaining_ms, &realtime);
		safe_cond_timedwait(&sim->state_changed, &sim->sim_mtx, &realtime);
	}
	if (!sim->end_sim && current_time_ms() >= phase_end)
		phase_finished = true;
	safe_mutex_handle(&sim->sim_mtx, M_UNLOCK);
	return (phase_finished);
}
