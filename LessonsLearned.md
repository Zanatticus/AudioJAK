# Lessons Learned

## HDMI Display
For the HDMI Display, the biggest lessons learned were in terms of abstraction layers and communication. It was very important from the beginning to map out exactly what should be done by each section of the HDMI display and what should be done by the audio player. In the end, we believe the correct decision was made by abstracting only the needed methods outside of the thread the HDMI uses. This allowed for a non-blocking approach that let the main audio player run as needed without any delay.