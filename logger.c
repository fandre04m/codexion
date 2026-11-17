/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   logger.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/17 15:02:10 by fandre-m          #+#    #+#             */
/*   Updated: 2026/11/17 15:02:10 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	log_state(t_coder *coder, char *msg, bool sim_lock_held)
{
	long long	elapsed;
	long long	now;

	if (!sim_lock_held)
		safe_mutex_handle(&coder->sim->sim_mtx, M_LOCK);
	if (coder->sim->end_sim)
	{
		if (!sim_lock_held)
			safe_mutex_handle(&coder->sim->sim_mtx, M_UNLOCK);
		return (0);
	}
	safe_mutex_handle(&coder->sim->output_mtx, M_LOCK);
	now = current_time_ms();
	elapsed = now - coder->sim->sim_start_time;
	printf("%lld %d %s\n", elapsed, coder->coder_id, msg);
	safe_mutex_handle(&coder->sim->output_mtx, M_UNLOCK);
	if (!sim_lock_held)
		safe_mutex_handle(&coder->sim->sim_mtx, M_UNLOCK);
	return (0);
}
