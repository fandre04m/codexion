/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   safe_thread.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/30 13:26:28 by fandre-m          #+#    #+#             */
/*   Updated: 2026/10/30 13:26:28 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	thread_error_handle(int status, t_thread_op op)
{
	const char	*msg;

	if (status == EAGAIN && op == T_CREATE)
		msg = "Insufficient resources to create another thread.";
	else if (status == EDEADLK && op == T_JOIN)
		msg = "A deadlock was detected or the thread ID passed to "
			"pthread_join specifies the calling thread.";
	else if (status == EINVAL && op == T_JOIN)
		msg = "Specified thread is not joinable or another "
			"thread is already waiting to join with this thread.";
	else if (status == ESRCH && op == T_JOIN)
		msg = "No thread with the supplied ID could be found.";
	else
		msg = "Something went wrong with the pthread handler.";
	return (error_msg(msg));
}

int	safe_pthread_create(pthread_t *thread, void *(*loop)(void *), void *coder)
{
	int	status;

	status = pthread_create(thread, NULL, loop, coder);
	if (status != 0)
		return (thread_error_handle(status, T_CREATE));
	return (status);
}

int	safe_pthread_join(pthread_t *thread)
{
	int	status;

	status = pthread_join(*thread, NULL);
	if (status != 0)
		return (thread_error_handle(status, T_JOIN));
	return (status);
}
