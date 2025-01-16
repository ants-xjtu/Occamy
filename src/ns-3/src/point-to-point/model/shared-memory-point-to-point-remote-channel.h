/*
 * Copyright (c) 2007 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: George Riley <riley@ece.gatech.edu>
 */

// This object connects two point-to-point net devices where at least one
// is not local to this simulator object.  It simply over-rides the transmit
// method and uses an MPI Send operation instead.

#ifndef SHARED_MEMORY_POINT_TO_POINT_REMOTE_CHANNEL_H
#define SHARED_MEMORY_POINT_TO_POINT_REMOTE_CHANNEL_H

#include "sharded-point-to-point-channel.h"

namespace ns3
{

/**
 * \ingroup point-to-point
 *
 * \brief A Remote Point-To-Point Channel
 *
 * This object connects two point-to-point net devices where at least one
 * is not local to this simulator object. It simply override the transmit
 * method and uses an MPI Send operation instead.
 */
class SharedMemoryPointToPointRemoteChannel : public SharedMemoryPointToPointChannel
{
  public:
    /**
     * \brief Get the TypeId
     *
     * \return The TypeId for this class
     */
    static TypeId GetTypeId();

    /**
     * \brief Constructor
     */
    SharedMemoryPointToPointRemoteChannel();

    /**
     * \brief Deconstructor
     */
    ~SharedMemoryPointToPointRemoteChannel() override;

    /**
     * \brief Transmit the packet
     *
     * \param p Packet to transmit
     * \param src Source SharedMemoryPointToPointNetDevice
     * \param txTime Transmit time to apply
     * \returns true if successful (currently always true)
     */
    bool TransmitStart(Ptr<const Packet> p, Ptr<SharedMemoryPointToPointNetDevice> src, Time txTime) override;
};

} // namespace ns3

#endif
