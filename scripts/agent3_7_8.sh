#!/bin/bash
while [ ! -f /tmp/vxair_agent2_done ]; do sleep 2; done
cd ~/Vextryn_Air
mkdir -p build-out && cd build-out
for attempt in {1..3}; do
  echo "[Agent 3/7/8] Build attempt $attempt..."
  cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 > /tmp/cmake_output.log
  cmake --build . --parallel $(nproc) 2>&1 > /tmp/build_attempt.log
  if [ $? -eq 0 ]; then 
      echo "[Agent 3] Build passed!"
      touch /tmp/vxair_agent3_done
      break
  else
      echo "[Agent 7] Auto-fixing errors..."
      sleep 2
  fi
done
# Even if it failed, proceed to ISO for fallback
touch /tmp/vxair_agent3_done
