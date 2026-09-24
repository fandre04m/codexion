/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/23 20:19:43 by fandre-m          #+#    #+#             */
/*   Updated: 2026/09/23 20:19:43 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <stdlib.h>

int	init_data(t_sim *sim)
{
	sim->end_sim = false;
	sim->dongles = malloc(sizeof(t_dongle) * sim->n_coders);
	if (!sim->dongles)
		return (error_msg("Dongles malloc failed."));
	sim->coders = malloc(sizeof(t_coder) * sim->n_coders);
	if (!sim->coders)
	{
		free(sim->dongles);
		sim->dongles = NULL;
		return (error_msg("Coders malloc failed."));
	}
	return (0);
}
