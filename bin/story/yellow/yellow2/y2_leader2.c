

#process "leader2"

class auto_att_left;
class auto_att_back;
class auto_move;
class auto_att_right;
class auto_att_fwd;
class auto_retro;

core_hex_B, 0, 
  {object_downlink, 0, 
    {component_peak, // component 1
      {object_uplink, 0},
      {object_pulse_xl:auto_att_left, 770},
      {object_pulse_xl:auto_att_right, -853},
      {object_downlink, -148, 
        {component_bowl, // component 2
          {object_uplink, 0},
          {object_none, 0},
          {object_none, 0},
          {object_pulse:auto_att_fwd, 1461},
          {object_pulse_xl:auto_att_fwd, 0},
        }
      },
      {object_downlink, 171, 
        {component_bowl, // component 3
          {object_uplink, 0},
          {object_none, 0},
          {object_none, 0},
          {object_pulse_xl:auto_att_fwd, 0},
          {object_pulse:auto_att_fwd, -1245},
        }
      },
    }
  },
  {object_downlink, 230, 
    {component_bowl, // component 4
      {object_uplink, 0},
      {object_none, 0},
      {object_none, 0},
      {object_slice:auto_att_right, 1030},
      {object_downlink, -194, 
        {component_cap, // component 5
          {object_interface, 0},
          {object_uplink, 0},
          {object_move:auto_move:auto_retro, 844},
          {object_move:auto_move:auto_retro, -171},
        }
      },
    }
  },
  {object_downlink, 421, 
    {component_snub, // component 6
      {object_uplink, 0},
      {object_pulse_l:auto_att_back, 0},
      {object_downlink, 0, 
        {component_cap, // component 7
          {object_move:auto_move, 809},
          {object_move:auto_move, -206},
          {object_interface, 0},
          {object_uplink, 0},
        }
      },
      {object_downlink, -766, 
        {component_cap, // component 8
          {object_uplink, 0},
          {object_interface, 0},
          {object_move:auto_move, 1324},
          {object_move:auto_move, 309},
        }
      },
      {object_none, 0},
    }
  },
  {object_repair_other, 0},
  {object_downlink, -421, 
    {component_snub, // component 9
      {object_uplink, 0},
      {object_downlink, 0, 
        {component_cap, // component 10
          {object_uplink, 0},
          {object_interface, 0},
          {object_move:auto_move, 206},
          {object_move:auto_move, -809},
        }
      },
      {object_pulse_l:auto_att_back, 0},
      {object_none, 0},
      {object_downlink, 766, 
        {component_cap, // component 11
          {object_move:auto_move, -309},
          {object_move:auto_move, -1324},
          {object_interface, 0},
          {object_uplink, 0},
        }
      },
    }
  },
  {object_downlink, -230, 
    {component_bowl, // component 12
      {object_uplink, 0},
      {object_none, 0},
      {object_none, 0},
      {object_downlink, 194, 
        {component_cap, // component 13
          {object_move:auto_move:auto_retro, 171},
          {object_move:auto_move:auto_retro, -844},
          {object_uplink, 0},
          {object_interface, 0},
        }
      },
      {object_slice:auto_att_left, -1030},
    }
  }

#code


enum
{
// These are the channels that processes in this stage
//  use to communicate with each other.
// There can be up to 8
CHANNEL_MAIN_BASE,
CHANNEL_TARGET,
CHANNEL_WELL,
CHANNEL_REQUEST_FOLLOWER,

};

enum
{
// these are codes for the first value of a broadcast message.
// they tell the recipient what kind of messge it is.
//  (the recipient needs the same enum declaration)
MESSAGE_MAIN_BASE, // the main base broadcasts a message which prevents other bases taking over as main base
MESSAGE_TARGET_FOUND,
MESSAGE_REQUEST_FOLLOWER,
MESSAGE_WELL_CLAIM
};

// Process AI modes (these reflect the capabilities of the process)
enum
{
  MODE_WANDER,
  MODE_ATTACK,
  MODES
};


// Targetting information
// Targetting memory allows processes to track targets (enemy or friend)
// The following enums are used as indices in the process' targetting memory
enum
{
  TARGET_PARENT, // a newly built process starts with its builder as entry 0
  TARGET_MAIN, // main target
  TARGET_FRONT, // target of directional forward attack
  TARGET_BACK,
  TARGET_LEFT,
  TARGET_RIGHT
};

int attacking_front; // is set to 1 if forward directional attack objects have a target
int front_attack_primary; // is set to 1 if forward directional attack objects are attacking
 // the primary target (e.g. target selected by command). Set to 0 if the objects are available for autonomous fire.
int attacking_back, attacking_left, attacking_right;

int other_target_x, other_target_y;

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
int saved_mode; // save the process' mode while it's attacking something it found

int move_x, move_y; // destination
int target_x, target_y; // location of target (to attack, follow etc)
int attack_x, attack_y;

int scan_result; // used to hold the results of a scan of nearby processes

int broadcast_count;

int initialised;
if (initialised == 0)
{
 initialised = 1;
 listen_channel(CHANNEL_TARGET); // scouts in this misson use this channel to broadcast "target found" messages
 mode = MODE_WANDER;
 special_AI(0, 203);
 gosub start_wandering;
}

front_attack_primary = 0; // this will be set to 1 if process is attacking its main target

if (broadcast_count <= 0)
{
// Because this is a leader, it periodically broadcasts its presence to any nearby followers:
 broadcast(4000, // range of broadcast (in pixels).
           CHANNEL_REQUEST_FOLLOWER, // channel - follower processes in this mission listen to channel 5
           0, // priority - 0 or 1. This message doesn't need high priority (which replaces low priority messages)
           MESSAGE_REQUEST_FOLLOWER); // message contents - code 40 means "please follow me"
 broadcast_count = 50;
}
 else
  broadcast_count --;

// What the process does next depends on its current mode
switch(mode)
{


 case MODE_WANDER:
  if (distance_from_xy(move_x, move_y) < 300)
  {
   gosub start_wandering;
   break;
  }
  gosub listen_for_broadcasts;
  if (scan_single(0,0,TARGET_MAIN,0,6,100,0) // 0 means any target
   || scan_single(0,0,TARGET_MAIN,0,0,100,0b1000)) // 0b1000 means only processes with allocator
  {
   special_AI(1, 6);
   mode = MODE_ATTACK;
   if (process[TARGET_MAIN].target_signature() & 0b1000) // 0b1000 means only processes with allocator
   {
        broadcast_target(-1, // range (-1 means unlimited range)
                      CHANNEL_TARGET, // channel (see the listen_channel(1) call in other process' code)
                      0, // priority (0 just means a 1 priority message will overwrite this one)
                      TARGET_MAIN, // this target is attached to the broadcast. A listener can retrieve it with get_message_target().
                      MESSAGE_TARGET_FOUND, // message contents
                      process[TARGET_MAIN].get_core_x(), // message contents. retrieved sequentially by read_message().
                      process[TARGET_MAIN].get_core_y()); // message contents
   
   
   }
   break;
  }
  auto_move.move_to(move_x, move_y);
  break;
  
 case MODE_ATTACK: // attacking something it found itself
  if (process[TARGET_MAIN].visible() <= 0) // target no longer exists, or is out of range
  {
   if (target_destroyed(TARGET_MAIN))
    special_AI(1, 11);
   auto_move.move_to(attack_x, attack_y);
   if (distance_from_xy_less(attack_x, attack_y, 600))
    gosub start_wandering;
   break;
  }
  attack_x = process[TARGET_MAIN].get_core_x();
  attack_y = process[TARGET_MAIN].get_core_y();
  auto_move.approach_target(TARGET_MAIN, 0, 600);
  auto_att_fwd.fire_at(TARGET_MAIN, 0);
  front_attack_primary = 1;
  break;
  
  
} // end of mode switch


if (get_damage() > 20
 && get_total_integrity() < 1000)
 special_AI(1, 4);

if (!front_attack_primary)
 auto_att_fwd.attack_scan(0, 400, TARGET_FRONT);

auto_att_left.attack_scan(-2048, 400, TARGET_LEFT);
auto_att_right.attack_scan(2048, 400, TARGET_RIGHT);
auto_att_back.attack_scan(4096, 400, TARGET_BACK);


// give next priority to charging interface:
charge_interface_max();

restore_self();

repair_self();

restore_scan(0,0); // scans for nearby processes with destroyed components and tries to restore them

repair_scan(0,0); // scans for nearby damaged processes and tries to repair them
  
exit;


start_wandering:
 mode = MODE_WANDER;
 move_x = 800 + random(world_x() - 1600);
 move_y = 800 + random(world_y() - 1600);
 return;



listen_for_broadcasts:
// listen for messages (the listen_channel() call above allows broadcasts to be received)
 if (next_message()
  && read_message() == MESSAGE_TARGET_FOUND)
 {
// Can assume that any message on CHANNEL_TARGET is a "target found" broadcast with the following format:
//  0: MESSAGE_TARGET_FOUND
//  1: target_x
//  2: target_y
// + an attached target.
  get_message_target(TARGET_MAIN);  
  attack_x = read_message();
  attack_y = read_message();
  mode = MODE_ATTACK;
 }
 return;
