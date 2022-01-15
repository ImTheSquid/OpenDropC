
// Recovered from sharingd`receiverSupportsX methods.
// A valid node needs to either have SUPPORTS_PIPELINING or SUPPORTS_MIXED_TYPES
// according to sharingd`[SDBonjourBrowser removeInvalidNodes:].
// Default flags on macOS: 0x3fb according to sharingd`[SDRapportBrowser defaultSFNodeFlags]
#define AIRDROP_SUPPORTS_URL 0x01
#define AIRDROP_SUPPORTS_DVZIP 0x02
#define AIRDROP_SUPPORTS_PIPELINING 0x04
#define AIRDROP_SUPPORTS_MIXED_TYPES 0x08
#define AIRDROP_SUPPORTS_UNKNOWN1 0x10
#define AIRDROP_SUPPORTS_UNKNOWN2 0x20
#define AIRDROP_SUPPORTS_IRIS 0x40
#define AIRDROP_SUPPORTS_DISCOVER_MAYBE 0x80
#define AIRDROP_SUPPORTS_UNKNOWN3 0x100
#define AIRDROP_SUPPORTS_ASSET_BUNDLE 0x200

typedef struct opendrop_config_s {
    
} opendrop_config;