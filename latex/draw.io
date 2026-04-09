<mxfile>
  <diagram name="ESP32 Flowchart">
    <mxGraphModel dx="1200" dy="800" grid="1" gridSize="10">
      <root>
        <mxCell id="0"/>
        <mxCell id="1" parent="0"/>

        <!-- Start -->
        <mxCell id="start" value="Start" style="ellipse;whiteSpace=wrap;html=1;" vertex="1" parent="1">
          <mxGeometry x="300" y="20" width="120" height="60" as="geometry"/>
        </mxCell>

        <!-- Init -->
        <mxCell id="init" value="Init WiFi, MQTT, DHT, LED" style="rounded=1;whiteSpace=wrap;html=1;" vertex="1" parent="1">
          <mxGeometry x="240" y="120" width="240" height="70" as="geometry"/>
        </mxCell>

        <!-- WiFi Check -->
        <mxCell id="wifi" value="WiFi Connected?" style="rhombus;whiteSpace=wrap;html=1;" vertex="1" parent="1">
          <mxGeometry x="280" y="230" width="160" height="100" as="geometry"/>
        </mxCell>

        <!-- Reconnect WiFi -->
        <mxCell id="wifi_re" value="Reconnect WiFi" style="rounded=1;whiteSpace=wrap;html=1;" vertex="1" parent="1">
          <mxGeometry x="60" y="240" width="160" height="70" as="geometry"/>
        </mxCell>

        <!-- MQTT Check -->
        <mxCell id="mqtt" value="MQTT Connected?" style="rhombus;whiteSpace=wrap;html=1;" vertex="1" parent="1">
          <mxGeometry x="280" y="380" width="160" height="100" as="geometry"/>
        </mxCell>

        <!-- Reconnect MQTT -->
        <mxCell id="mqtt_re" value="Reconnect MQTT" style="rounded=1;whiteSpace=wrap;html=1;" vertex="1" parent="1">
          <mxGeometry x="60" y="390" width="160" height="70" as="geometry"/>
        </mxCell>

        <!-- Read -->
        <mxCell id="read" value="Read DHT22 (Temp, Humidity)" style="rounded=1;whiteSpace=wrap;html=1;" vertex="1" parent="1">
          <mxGeometry x="240" y="540" width="240" height="70" as="geometry"/>
        </mxCell>

        <!-- Publish -->
        <mxCell id="pub" value="Publish JSON → farm/sensor" style="rounded=1;whiteSpace=wrap;html=1;" vertex="1" parent="1">
          <mxGeometry x="240" y="640" width="240" height="70" as="geometry"/>
        </mxCell>

        <!-- Delay -->
        <mxCell id="delay" value="Delay 5s" style="rounded=1;whiteSpace=wrap;html=1;" vertex="1" parent="1">
          <mxGeometry x="300" y="740" width="120" height="60" as="geometry"/>
        </mxCell>

        <!-- Connections -->
        <mxCell edge="1" source="start" target="init" parent="1"/>
        <mxCell edge="1" source="init" target="wifi" parent="1"/>

        <!-- WiFi Decision -->
        <mxCell edge="1" source="wifi" target="wifi_re" parent="1" value="No"/>
        <mxCell edge="1" source="wifi_re" target="wifi" parent="1"/>
        <mxCell edge="1" source="wifi" target="mqtt" parent="1" value="Yes"/>

        <!-- MQTT Decision -->
        <mxCell edge="1" source="mqtt" target="mqtt_re" parent="1" value="No"/>
        <mxCell edge="1" source="mqtt_re" target="mqtt" parent="1"/>
        <mxCell edge="1" source="mqtt" target="read" parent="1" value="Yes"/>

        <!-- Main Flow -->
        <mxCell edge="1" source="read" target="pub" parent="1"/>
        <mxCell edge="1" source="pub" target="delay" parent="1"/>

        <!-- Loop -->
        <mxCell edge="1" source="delay" target="wifi" parent="1"/>

      </root>
    </mxGraphModel>
  </diagram>
</mxfile>