<?xml version="1.0" encoding="UTF-8"?>
<tileset name="mario" tilewidth="91" tileheight="85" tilecount="12" columns="4">
 <properties>
  <property name="state" value="mid"/>
 </properties>
 <image source="left_tiles.png" trans="ff24b6" width="364" height="255"/>
 <tile id="1" type="low">
  <properties>
   <property name="state" value="low"/>
  </properties>
  <animation>
   <frame tileid="0" duration="100"/>
   <frame tileid="1" duration="100"/>
  </animation>
 </tile>
 <tile id="2" type="up">
  <properties>
   <property name="state" value="up"/>
  </properties>
  <animation>
   <frame tileid="2" duration="100"/>
   <frame tileid="3" duration="100"/>
  </animation>
 </tile>
 <tile id="4" type="mid">
  <properties>
   <property name="state" value="mid"/>
  </properties>
  <animation>
   <frame tileid="4" duration="100"/>
   <frame tileid="5" duration="100"/>
  </animation>
 </tile>
 <tile id="6" type="miss">
  <properties>
   <property name="state" value="miss"/>
  </properties>
  <animation>
   <frame tileid="6" duration="100"/>
   <frame tileid="7" duration="100"/>
   <frame tileid="6" duration="100"/>
   <frame tileid="7" duration="100"/>
   <frame tileid="6" duration="100"/>
   <frame tileid="7" duration="100"/>
  </animation>
 </tile>
 <tile id="8" type="rest">
  <properties>
   <property name="state" value="rest"/>
  </properties>
  <animation>
   <frame tileid="8" duration="100"/>
   <frame tileid="9" duration="100"/>
   <frame tileid="8" duration="100"/>
   <frame tileid="9" duration="100"/>
   <frame tileid="8" duration="100"/>
   <frame tileid="9" duration="100"/>
   <frame tileid="10" duration="100"/>
  </animation>
 </tile>
</tileset>
