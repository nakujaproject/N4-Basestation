# XCTU Configuration Profiles

This folder contains XCTU profile exports (.xpro files) for quick module configuration.

## Profiles

- [ ] **Sender Profile** - Rocket/transmitter configuration
- [ ] **Receiver Profile** - Ground station configuration
- [ ] **Test Profile** - Range testing configuration

## How to Use

1. Open XCTU
2. Connect XBee module
3. Click **Load Profile** button
4. Select the appropriate .xpro file
5. Click **Write** to apply configuration

## Profile Details

### Sender (Rocket)
- Mode: Transparent (AT)
- Baud: 115200
- Power: Maximum (PL=4)
- PAN ID: 7777
- HP: Custom

### Receiver (Ground Station)
- Mode: Transparent (AT)
- Baud: 115200
- Power: Maximum (PL=4)
- PAN ID: 7777 (must match sender)
- HP: Custom (must match sender)

**Note:** Existing profiles already present in logged sessions folder will be documented here.
