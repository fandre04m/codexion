/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   stopper.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/17 19:10:55 by fandre-m          #+#    #+#             */
/*   Updated: 2026/11/17 19:10:55 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

bool	stop_handler(t_sim *sim, t_stop_reason reason, bool sim_mtx_held)
{
	bool	stop_recorded;

	if (!sim_mtx_held)
		safe_mutex_handle(&sim->sim_mtx, M_LOCK);
	stop_recorded = false;
	if (!sim->end_sim)
	{
		sim->end_sim = true;
		sim->stop_reason = reason;
		safe_cond_handle(&sim->state_changed, C_BROADCAST);
		stop_recorded = true;
	}
	if (!sim_mtx_held)
		safe_mutex_handle(&sim->sim_mtx, M_UNLOCK);
	return (stop_recorded);
}
