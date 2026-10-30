/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   safe_mutex.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/26 18:14:02 by fandre-m          #+#    #+#             */
/*   Updated: 2026/10/28 21:23:06 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	mutex_error_handle(int status, t_mutex_op op)
{
	const char	*msg;

	if (status == EAGAIN && op == M_INIT)
		msg = "System lacked the necessary resources (other than memory)"
			" to initialize another mutex.";
	else if (status == ENOMEM && op == M_INIT)
		msg = "Insufficient memory exists to initialize the mutex.";
	else if (status == EPERM && op == M_INIT)
		msg = "Caller does not have the privilege to perform the operation.";
	else if (op == M_DESTROY)
		msg = "Unable to destroy mutex.";
	else if (op == M_LOCK)
		msg = "Unable to lock mutex.";
	else if (op == M_UNLOCK)
		msg = "Unable to unlock mutex.";
	else
		msg = "Something went wrong in the mutex handlers.";
	return (error_msg(msg));
}

int	safe_mutex_handle(t_mtx *mutex, t_mutex_op op)
{
	int	status;

	if (op == M_INIT)
		status = pthread_mutex_init(mutex, NULL);
	else if (op == M_LOCK)
		status = pthread_mutex_lock(mutex);
	else if (op == M_UNLOCK)
		status = pthread_mutex_unlock(mutex);
	else if (op == M_DESTROY)
		status = pthread_mutex_destroy(mutex);
	else
		return (error_msg("Operation code incompatible with mutex functions.\n"
				G "Valid codes: M_INIT M_LOCK M_UNLOCK M_DESTROY." RST));
	if (status != 0)
		return (mutex_error_handle(status, op));
	return (status);
}
