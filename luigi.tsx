<?xml version="1.0" encoding="UTF-8"?>
<tileset name="luigi" tilewidth="99" tileheight="76" tilecount="12" columns="4">
 <image source="right_tiles.png" trans="ff24b6" width="396" height="228"/>
 <tile id="1" type="low">
  <properties>
   <property name="state" value="low"/>
  </properties>
  <objectgroup draworder="index"/>
  <animation>
   <frame tileid="0" duration="100"/>
   <frame tileid="1" duration="100"/>
  </animation>
 </tile>
 <tile id="2" type="mid">
  <properties>
   <property name="state" value="mid"/>
  </properties>
  <objectgroup draworder="index"/>
  <animation>
   <frame tileid="2" duration="100"/>
   <frame tileid="3" duration="100"/>
  </animation>
 </tile>
 <tile id="4" type="miss">
  <properties>
   <property name="state" value="miss"/>
  </properties>
  <animation>
   <frame tileid="4" duration="100"/>
   <frame tileid="5" duration="100"/>
   <frame tileid="4" duration="100"/>
   <frame tileid="5" duration="100"/>
   <frame tileid="4" duration="100"/>
   <frame tileid="5" duration="100"/>
  </animation>
 </tile>
 <tile id="6" type="rest">
  <properties>
   <property name="state" value="rest"/>
  </properties>
  <animation>
   <frame tileid="6" duration="100"/>
   <frame tileid="7" duration="100"/>
   <frame tileid="6" duration="100"/>
   <frame tileid="7" duration="100"/>
   <frame tileid="6" duration="100"/>
   <frame tileid="7" duration="100"/>
   <frame tileid="5" duration="100"/>
  </animation>
 </tile>
 <tile id="9" type="up">
  <properties>
   <property name="state" value="up"/>
  </properties>
  <animation>
   <frame tileid="9" duration="100"/>
   <frame tileid="10" duration="100"/>
  </animation>
 </tile>
</tileset>
