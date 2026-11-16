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

int	init_data(t_sim *sim)
{
	if (safe_mutex_handle(&sim->sim_mtx, M_INIT))
		return (1);
	if (safe_cond_handle(&sim->state_changed, C_INIT))
		return (1);
	if (safe_mutex_handle(&sim->output_mtx, M_INIT))
		return (1);
	sim->end_sim = false;
	sim->stop_reason = STOP_NONE;
	sim->dongles = malloc(sizeof(t_dongle) * sim->n_coders);
	if (!sim->dongles)
		return (error_msg("Dongles malloc failed.", 1));
	sim->coders = malloc(sizeof(t_coder) * sim->n_coders);
	if (!sim->coders)
		return (error_msg("Coders malloc failed.", 1));
	return (0);
}
