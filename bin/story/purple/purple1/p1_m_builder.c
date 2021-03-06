
#process "m_builder"

class auto_move;
class auto_att_back;
class auto_att_fwd;

core_pent_A, 0, 
  {object_repair, 0},
  {object_downlink, -756, 
    {component_bowl, // component 1
      {object_uplink, 0},
      {object_none, 0},
      {object_pulse:auto_att_back, 0},
      {object_pulse:auto_att_fwd, 0},
      {object_none, 0},
    }
  },
  {object_downlink, -873, 
    {component_cap, // component 2
      {object_move:auto_move, 992},
      {object_move:auto_move, 0},
      {object_downlink, -610, 
        {component_cap, // component 3
          {object_uplink, 0},
          {object_interface, 0},
          {object_move:auto_move, 1195},
          {object_move:auto_move, 180},
        }
      },
      {object_uplink, 0},
    }
  },
  {object_downlink, 873, 
    {component_cap, // component 4
      {object_uplink, 0},
      {object_downlink, 610, 
        {component_cap, // component 5
          {object_move:auto_move, -180},
          {object_move:auto_move, -1195},
          {object_interface, 0},
          {object_uplink, -1070},
        }
      },
      {object_move:auto_move, 0},
      {object_move:auto_move, -992},
    }
  },
  {object_downlink, 756, 
    {component_bowl, // component 6
      {object_uplink, 0},
      {object_pulse:auto_att_back, 0},
      {object_build, 0},
      {object_none, 0},
      {object_pulse:auto_att_fwd, 0},
    }
  },

#code


enum
{
  MODE_SEEK_WELL, // process is wandering randomly looking for a well
  MODE_BUILD_BASE, // process is trying to build a base near a data well
  MODES
};

// Targetting information
// Targetting memory allows processes to track targets (enemy or friend)
// The following enums are used as indices in the process' targetting memory
enum
{
TARGET_PARENT, // parent process automatically stored in address 0
TARGET_BACK,
TARGET_FRONT
};


enum
{
TEMPLATE_BASE,
TEMPLATE_BUILDER,
TEMPLATE_HARVESTER,
TEMPLATE_OUTPOST,

TEMPLATE_FLAGSHIP1,
TEMPLATE_FLAGSHIP2,

TEMPLATE_ESCORT,
TEMPLATE_PICKET


};

enum
{
// These are the channels that processes in this stage
//  use to communicate with each other.
// There can be up to 8
CHANNEL_MAIN_BASE,
CHANNEL_FLAGSHIP,
CHANNEL_TARGET,
CHANNEL_WELL,
CHANNEL_REQUEST_FOLLOWER,
CHANNEL_HELP
};

enum
{
// these are codes for the first value of a broadcast message.
// they tell the recipient what kind of messge it is.
//  (the recipient needs the same enum declaration)
MESSAGE_MAIN_BASE, // the main base broadcasts a message which prevents other bases taking over as main base
MESSAGE_TARGET_FOUND,
MESSAGE_FLAGSHIP,
MESSAGE_WELL_CLAIM,
MESSAGE_FORMATION,
MESSAGE_REQUEST_FOLLOWER,
MESSAGE_UNDER_ATTACK
};



// Variable declaration and initialisation
//  (note that declaration and initialisation cannot be combined)
//  (also, variables retain their values between execution cycles)
int core_x, core_y; // location of core
core_x = get_core_x(); // location is updated each cycle
core_y = get_core_y();
int angle; // direction process is pointing
 // angles are in integer degrees from 0 to 8192, with 0 being right,
 // 2048 down, 4096 left and 6144 up.
angle = get_core_angle(); // angle is updated each cycle

int mode; // what is the process doing? (should be one of the MODE enums)
int scan_result;

int move_x, move_y; // destination

// Harvester variables
int data_well_x, data_well_y; // location of data well
int ignore_well_x, ignore_well_y; // ignore data well with a friendly allocator near it
int build_angle;
int broadcast_count;

int initialised;

if (initialised == 0)
{
 initialised = 1;
 listen_channel(CHANNEL_WELL); // listen to harvesters' well claim broadcasts
 mode = MODE_SEEK_WELL;
 special_AI(0, 405);
 gosub start_wandering;
}


if (broadcast_count <= 0)
{
// this process periodically broadcasts its presence to any nearby followers:
 broadcast(1000, // range of broadcast (in pixels).
           CHANNEL_REQUEST_FOLLOWER, // channel - follower processes in this mission listen to channel 5
           0, // priority - 0 or 1. This message doesn't need high priority (which replaces low priority messages)
           MESSAGE_REQUEST_FOLLOWER); // message contents
 broadcast_count = 50;
}
 else
  broadcast_count --;


// What the process does next depends on its current mode
switch(mode)
{

case MODE_SEEK_WELL:
  if (distance_from_xy(move_x, move_y) < 300)
  {
   gosub start_wandering;
   break;
  }
  auto_move.move_to(move_x, move_y);
// listen to broadcasts from harvesters
  if (next_message()
   && read_message() == MESSAGE_WELL_CLAIM)
  {
   mode = MODE_BUILD_BASE;
   data_well_x = read_message();
   data_well_y = read_message();
   build_angle = random(8192);
   add_to_build_queue(TEMPLATE_OUTPOST, 
                      data_well_x + cos(build_angle, 500), 
                      data_well_y + sin(build_angle, 500), 
                      build_angle, 
                      0,
                      0);  
   break;
  } 
// search for nearby wells  
  if (search_for_well() // found nearby well!
   && (get_well_x() != ignore_well_x
    || get_well_y() != ignore_well_y))
  {
   mode = MODE_BUILD_BASE;
   data_well_x = get_well_x();
   data_well_y = get_well_y();
   build_angle = random(8192);
   add_to_build_queue(TEMPLATE_OUTPOST, 
                      data_well_x + cos(build_angle, 500), 
                      data_well_y + sin(build_angle, 500), 
                      build_angle, 
                      0,
                      0);
   break;
  }
  break;
  
case MODE_BUILD_BASE:
  auto_move.move_to(data_well_x, data_well_y);
  if (distance_from_xy(data_well_x, data_well_y) < 300)
  {
// first check for a friendly base already near the well.
// If there is one, ignore this well and try to find another.  
   scan_result = scan_single(0, // x_offset
                             0, // y_offset
                             -1, // process memory. -1 means discard target
                             2, // 2 = accept friendly only
                             0, // components min
                             100, // components max (100 is too high so this will always pass)
                             0b1000); // bitfield for accepting processes with allocators
   if (scan_result > 0)
   {
    ignore_well_x = data_well_x;
    ignore_well_y = data_well_y;
    mode = MODE_SEEK_WELL;
    cancel_build_queue();
    gosub start_wandering;
    break;
   }
// there doesn't appear to be a nearby friendly base. So let's build one!
// the call to add_to_build_queue above should have left a base on the build queue, so we can try to build it:
   int build_result;
   build_result = build_from_queue(-1); // -1 means not to save built process as target
   if (build_result == 1)
   {
    ignore_well_x = data_well_x;
    ignore_well_y = data_well_y;
    mode = MODE_SEEK_WELL;
// don't need to call cancel_build_queue() here as build_from_queue removes the built process from the queue
    gosub start_wandering;
    break;    
   }
  }
  break;

} // end of mode switch


auto_att_back.attack_scan(4096, 300, TARGET_BACK);
auto_att_fwd.attack_scan(0, 300, TARGET_FRONT);

charge_interface_max();

restore_self();
repair_self();

exit;


start_wandering:
 move_x = 800 + random(world_x() - 1600);
 move_y = 800 + random(world_y() - 1600);
 return;
