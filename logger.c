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

int	log_state(t_coder *coder, char *msg)
{
	long long	elapsed;
	long long	now;

	safe_mutex_handle(&coder->sim->sim_mtx, M_LOCK);
	if (coder->sim->end_sim)
	{
		safe_mutex_handle(&coder->sim->sim_mtx, M_UNLOCK);
		return (0);
	}
	now = current_time_ms();
	if (now == -1)
	{
		safe_mutex_handle(&coder->sim->sim_mtx, M_UNLOCK);
		return (1);
	}
	elapsed = now - coder->sim->sim_start_time;
	safe_mutex_handle(&coder->sim->output_mtx, M_LOCK);
	printf("%lld %d %s\n", elapsed, coder->coder_id, msg);
	safe_mutex_handle(&coder->sim->output_mtx, M_UNLOCK);
	safe_mutex_handle(&coder->sim->sim_mtx, M_UNLOCK);
	return (0);
}
