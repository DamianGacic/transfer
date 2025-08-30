#pragma once

extern volatile bool	sigint_pressed; //Crtl + C global
void	set_sigint( void ); //establish sigint handler