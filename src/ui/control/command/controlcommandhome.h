#ifndef CONTROLCOMMANDHOME_H
#define CONTROLCOMMANDHOME_H

#include "controlcommandbase.h"

class ControlCommandHome : public ControlCommandBase
{
public:
    static bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);
};

#endif // CONTROLCOMMANDHOME_H
