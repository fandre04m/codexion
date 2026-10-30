/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   safe_cond.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/28 17:27:42 by fandre-m          #+#    #+#             */
/*   Updated: 2026/10/29 01:47:01 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	cond_error_handle(int status, t_cond_op op)
{
	const char	*msg;

	if (status == EAGAIN && op == C_INIT)
		msg = "System lacked the necessary resources (other than memory) "
			"to initialize another condition variable.";
	else if (status == ENOMEM && op == C_INIT)
		msg = "Insufficient memory to initialize the condition variable.";
	else if (status == ENOTRECOVERABLE && op == C_WAIT)
		msg = "The state protected by the mutex is not recoverable.";
	else if (status == ENOTRECOVERABLE && op == C_TIMEDWAIT)
		msg = "The state protected by the mutex is not recoverable.";
	else if (status == ETIMEDOUT && op == C_TIMEDWAIT)
		msg = "The time specified by abstime to cond_timedwait has passed.";
	else if (status == EINVAL && op == C_TIMEDWAIT)
		msg = "The abstime argument specified a nanoseconds "
			"value < 0 or >= 1000 million.";
	else if (op == C_DESTROY)
		msg = "Unable do destroy condition variable.";
	else if (op == C_SIGNAL)
		msg = "cond_signal failed.";
	else if (op == C_BROADCAST)
		msg = "cond_broadcast failed.";
	else
		msg = "Something went wrong with the pthread condition handler.";
	return (error_msg(msg));
}

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
		return (error_msg("Operation code incompatible with condition "
				"variable functions.\n"
				G "Valid codes: C_INIT C_DESTROY C_SIGNAL C_BROADCAST." RST));
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
	if (status != 0)
		return (cond_error_handle(status, C_TIMEDWAIT));
	return (status);
}
