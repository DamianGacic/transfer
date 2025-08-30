#include <iostream>
#include <csignal>

volatile bool	sigint_pressed;

void	sigint_handler( int signal )
{
	(void) signal;
	sigint_pressed = true;
}

void	set_sigint( void )
{
	sigint_pressed = false;
	signal(SIGINT, sigint_handler);
}
