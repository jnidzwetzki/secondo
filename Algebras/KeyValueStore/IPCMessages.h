/*
----
This file is part of SECONDO.

Copyright (C) 2015,
Faculty of Mathematics and Computer Science,
Database Systems for New Applications.

SECONDO is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

SECONDO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with SECONDO; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
----

*/

#ifndef IPCMESSAGES_H_
#define IPCMESSAGES_H_

enum IPCMessage {
  IPC_MSG_RESULT,
  IPC_MSG_ADDCONNECTION,
  IPC_MSG_REMOVECONNECTION,
  IPC_MSG_UPDATESERVERLIST,
  IPC_MSG_RETRYCONNECTION,
  IPC_MSG_SERVERINFORMATIONSTRING,
  IPC_MSG_SETDATABASE,
  IPC_MSG_USEDATABASE,
  IPC_MSG_DISTRIBUTIONREF,
  IPC_MSG_DISTRIBUTIONREFSET,

  IPC_MSG_TRANSFERID,
  IPC_MSG_GLOBALTUPELID,
  IPC_MSG_STARTCLIENT,
  IPC_MSG_STOPCLIENT,
  IPC_MSG_SETID,
  IPC_MSG_DISTRIBUTIONPOINTID,
  IPC_MSG_INITCLIENTS,
  IPC_MSG_SETMASTER,
  IPC_MSG_SYNCSERVERLIST,
  IPC_MSG_TRYRESTRUCTURELOCK,
  IPC_MSG_UPDATERESTRUCTURELOCK,
  IPC_MSG_UNLOCKRESTRUCTURELOCK,
  IPC_MSG_REQUESTDISTRIBUTIONELEM,
  IPC_MSG_EXECCOMMAND,
  IPC_MSG_DISTRIBUTIONDATA,

  IPC_MSG_ADDDISTRIBUTIONELEM,
  IPC_MSG_ADDDISTRIBUTIONRECT,
  IPC_MSG_ADDDISTRIBUTIONINT,
  IPC_MSG_FILTERDISTRIBUTION,

  IPC_MSG_QTDISTINCT,

  IPC_MSG_INITDISTRIBUTE,
  IPC_MSG_SENDTUPLE,
  IPC_MSG_ERROR,
  IPC_MSG_ENDDISTRIBUTE,
  IPC_MSG_REQUESTRESULT,
  IPC_MSG_CLOSEDISTRIBUTE,

  IPC_MSG_INITNETWORKSTREAM,
  IPC_MSG_REQUESTSTREAMTYPE,
  IPC_MSG_REQUESTSTREAM,
  IPC_MSG_REMOVESTREAM,

  IPC_MSG_CLOSECONNECTION
};

#endif /* IPCMESSAGES_H_ */