# Setup-page security boundaries

The setup AP is open and configuration is served over plain HTTP. It exists
with blank or invalid replacement settings, including after a first flash or
Factory reset.
Once valid Wi-Fi settings are saved, normal and scheduled wakes use station
mode only; there is no AP fallback when association fails. A device with direct
network access can still obtain the bootstrap token and change settings or
invoke Factory reset while the clock is awake. Do not expose the clock through
port forwarding.

The reviewed source checks the HTTP Host and, when supplied, exact Origin.
State-changing requests require a fresh per-boot token sent in a custom header.
The page-close beacon uses a form-body token because beacon cannot set that
header. Tokens are not placed in URLs. Responses are not cacheable and the
setup page disallows framing.

These controls reduce cross-site request forgery and hostile-host requests.
They do **not** authenticate an owner, encrypt a Wi-Fi password in transit,
prevent a nearby person joining the open AP, or stop an on-network attacker
who directly fetches the bootstrap configuration. No password is returned by
the configuration endpoint, but settings remain stored on the device.

The station-only policy reduces routine RF exposure, but it is not an access
control mechanism. Use the clock only on a trusted local network and do not
expose it through port forwarding. Provisioning/access authentication is not
provided by this build.

Factory reset clears this firmware's stored settings and explicitly clears SDK
station credentials that an application-only installation may leave behind.
It is not a guarantee that every historical flash byte has been securely erased. Raw flash dumps
and serial captures must be treated as private. Public photos are sanitised
copies, not the location-bearing originals.

The initial v1.0.0 build was checked through the phone captive portal, AP
address and LAN address. No OTA endpoint is included: MM32-controlled
reset/wake windows make uninterrupted updates an unvalidated assumption.

Reference: [OWASP CSRF prevention guidance](https://cheatsheetseries.owasp.org/cheatsheets/Cross-Site_Request_Forgery_Prevention_Cheat_Sheet.html).
