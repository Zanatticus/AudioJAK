# Lessons Learned

## HDMI Display
For the HDMI Display, the biggest lessons learned were in terms of abstraction layers and communication. It was very important from the beginning to map out exactly what should be done by each section of the HDMI display and what should be done by the audio player. In the end, we believe the correct decision was made by abstracting only the needed methods outside of the thread the HDMI uses. This allowed for a non-blocking approach that let the main audio player run as needed without any delay.

## Audio Player
We learned that it was extremely important to think about the integration of components early on in the design process. By planning early on how the integration may work, the actual integration takes less time and debugging. Communication plays a crucial role in this as with good communication, teammates will know exactly what to expect from each others’ components. It is also important to not save integration for later on in the project. Having integration between unfinished components earlier on in the design timeline, helps facilitate conversations as to how to improve individual components for the final implementation. Once a minimal product, with integration of individual components, is made it is much easier to later add features to it. 
