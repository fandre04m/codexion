/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   scheduler.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/19 16:02:46 by fandre-m          #+#    #+#             */
/*   Updated: 2026/11/19 16:02:46 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

bool	has_priority(t_coder *a, t_coder *b)
{
	if (strcmp(a->sim->scheduler, "fifo"))
		return (a->req_order < b->req_order);
	else
		if (a->deadline != b->deadline)
			return (a->deadline < b->deadline);
		if (a->req_order != b->req_order)
			return (a->req_order < b->req_order);
		return (a->coder_id < b->coder_id);
}
