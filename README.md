# InstDrv
Win Driver Installer

# toolkit to load windows .sys driver
equalent steps as follows
  1. sc create helloworld binPath=helloworld.sys type=kernel start=auto
  2. sc start helloworld
  3. sc stop helloworld
  4. sc delete helloworld
  
 # features
  1. dragfile supported.
