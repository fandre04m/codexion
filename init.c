/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/23 20:19:43 by fandre-m          #+#    #+#             */
/*   Updated: 2026/11/11 18:55:55 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	init_dongles(t_sim *sim)
{
	int			i;
	t_dongle	*dongle;

	i = -1;
	while (++i < sim->n_coders)
	{
		dongle = sim->dongles + i;
		dongle->dongle_id = i + 1;
		dongle->in_use = false;
		dongle->ready_at = 0;
		if (safe_mutex_handle(&dongle->dongle_mtx, M_INIT))
			return (1);
		sim->dongle_init = i + 1;
	}
	return (0);
}

int	init_data(t_sim *sim)
{
	if (safe_mutex_handle(&sim->sim_mtx, M_INIT))
		return (1);
	sim->clean_stage = CLEAN_SIM_MTX;
	if (safe_cond_handle(&sim->state_changed, C_INIT))
		return (1);
	sim->clean_stage = CLEAN_COND;
	if (safe_mutex_handle(&sim->output_mtx, M_INIT))
		return (1);
	sim->clean_stage = CLEAN_OUTPUT;
	sim->end_sim = false;
	sim->stop_reason = STOP_NONE;
	sim->dongles = malloc(sizeof(t_dongle) * sim->n_coders);
	if (!sim->dongles)
		return (error_msg("Dongles malloc failed.", 1));
	sim->coders = malloc(sizeof(t_coder) * sim->n_coders);
	if (!sim->coders)
		return (error_msg("Coders malloc failed.", 1));
	if (init_dongles(sim))
		return (1);
	init_coders(sim);
	return (0);
}
