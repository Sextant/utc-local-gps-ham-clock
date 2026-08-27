# Contributing

Bug reports, documentation corrections, hardware observations, and proposed
noncommercial improvements are welcome through GitHub Issues and pull requests.

Before proposing a firmware change:

1. Start from the current modular sketch and preserve the numbered-tab layout.
2. Describe the exact CYD and GPS hardware revision used for testing.
3. Keep display rectangles and touch hit regions consistent.
4. Check all GPIO assignments for conflicts with TFT, touch, SD, GPS, PPS, R21,
   and backlight PWM.
5. Compile using the documented board/core/library versions and report flash
   and memory usage.
6. Test settings persistence, GPS acquisition and failure paths, SD-card lookup,
   Night Mode, alarm behavior, and all touched screens on physical hardware.
7. Do not commit generated databases, downloaded datasets, credentials, or
   personal coordinates.

Contributions must be compatible with the repository's PolyForm Noncommercial
License 1.0.0 and must preserve third-party notices. Do not submit code or data
that you do not have permission to redistribute. By submitting a contribution,
you represent that you have the right to do so under the project's terms.

Commercial licensing requests are not feature requests; follow
`COMMERCIAL-LICENSING.md`.

