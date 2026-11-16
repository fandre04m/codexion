/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cleaner.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/16 11:36:05 by fandre-m          #+#    #+#             */
/*   Updated: 2026/11/16 11:36:05 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	clean_dongles(t_sim *sim)
{
	int	i;
	int	status;

	i = -1;
	status = 0;
	while (++i < sim->dongle_init)
		if (safe_mutex_handle(&sim->dongles[i].dongle_mtx, M_DESTROY))
			status = 1;
	return (status);
}

int	clean_sim(t_sim *sim)
{
	int	status;

	status = 0;
	if (sim->clean_stage >= CLEAN_OUTPUT)
		if (safe_mutex_handle(&sim->output_mtx, M_DESTROY))
			status = 1;
	if (sim->clean_stage >= CLEAN_COND)
		if (safe_cond_handle(&sim->state_changed, C_DESTROY))
			status = 1;
	if (sim->clean_stage >= CLEAN_SIM_MTX)
		if (safe_mutex_handle(&sim->sim_mtx, M_DESTROY))
			status = 1;
	if (clean_dongles(sim))
		status = 1;
	free(sim->dongles);
	free(sim->coders);
	return (status);
}
