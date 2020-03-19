#ifndef SONIC_H
#define SONIC_H

#define MODE_GROUND (2)
#define MODE_RWALL (3)
#define MODE_CEILING (0)
#define MODE_LWALL (1)

void sonic_init(void);
void sonic_move(void);
void sonic_collision(void); 
void sonic_display(void);

#endif
