/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   safe_cond.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/28 17:27:42 by fandre-m          #+#    #+#             */
/*   Updated: 2026/10/28 21:35:49 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	safe_cond_handle(t_cond *cond, t_cond_op op)
{
	int	status;

	if (op == C_INIT)
		status = pthread_cond_init(cond, NULL);
	else if (op == C_DESTROY)
		status = pthread_cond_destroy(cond);
	else if (op == C_SIGNAL)
		status = pthread_cond_signal(cond);
	else if (op == C_BROADCAST)
		status = pthread_cond_broadcast(cond);
	else
		return (error_msg("Operation code incompatible with condition funvtions.\n"
					G "Valid codes: C_INIT, C_DESTROY, C_SIGNAL, C_BROADCAST." RST));
	if (status != 0)
		return (cond_error_handle(status, op));
	return (status);
}

int	safe_cond_wait(t_cond *cond, t_mtx *mutex)
{
	int	status;

	status = pthread_cond_wait(cond, mutex);
	if (status != 0)
		return (cond_error_handle(status, C_WAIT));
	return (status);
}

int	safe_cond_timedwait(t_cond *cond, t_mtx *mutex,
		const struct timespec *deadline)
{
	int	status;

	status = pthread_cond_timedwait(cond, mutex, deadline);
	return (status);
}
