# **Data Marble Machine: A Visual and Interactive Approach to Energy Awareness**

<p align="center"> <img src="https://raw.githubusercontent.com/Lionel-Lim/data-marble-machine/main/report/resource/marblemachine.jpeg" width="500" alt="Marble Machine on a plinth">

## **Project Overview**

This project showcases the innovative "Marble Machine," a device bridging the gap between art and technology. By converting real-time energy consumption data into tangible movement of marbles, this device provides a captivating visual and auditory experience. Accompanied by a user-friendly web application, this project seeks to revolutionise the way users interact with their energy consumption data.

<p align="center"> <img src="https://raw.githubusercontent.com/Lionel-Lim/data-marble-machine/main/report/resource/marblemachine_top.jpeg" width="500" alt="Marble machine on a pillar">

### **Key Features**

- Transforms real-time energy consumption into marble movement.
- Provides both visual and auditory feedback.
- Created using modern techniques like laser cutting and 3D printing.
- Accompanying web application for detailed energy data insights.

## **Getting Started**

### **Prerequisites**

- Ensure you have [Flutter](https://flutter.dev/docs/get-started/install) installed for web application setup.
- Necessary hardware components for the Marble Machine.
- Basic understanding of IoT device operation.

### **Application Installation**

1. **Clone the Repository**

```
git clone https://github.com/Lionel-Lim/data-marble-machine
```

1. Navigate to the cloned directory.
2. Follow the instructions in the 'How to Make' section - Web Application Setup.

## **How to Make**

<p align="center"> <img src="https://raw.githubusercontent.com/Lionel-Lim/data-marble-machine/main/report/resource/printedcomponents.jpeg" width="500" alt="Printed materials">

1. **Required Components, Materials and Machines**
- **Components**
    - [Adafruit ESP32 Feather V2 - 8MB Flash + 2 MB PSRAM - STEMMA QT](https://www.adafruit.com/product/5400 "https://www.adafruit.com/product/5400")
    - [Ultra Skinny NeoPixel 1515 LED Strip 4mm wide (0.5 meter long - 75 LEDs)](https://thepihut.com/products/ultra-skinny-neopixel-1515-led-strip-4mm-wide "https://thepihut.com/products/ultra-skinny-neopixel-1515-led-strip-4mm-wide")
    - [DC Motor + Stepper FeatherWing Add-on For All Feather Boards](https://www.adafruit.com/product/2927 "https://www.adafruit.com/product/2927")
    - [Micro Mini N20 DC Motor with gear box](https://www.amazon.co.uk/HALJIA-300RPM-Reducing-Reduction-Terminals/dp/B07YB76TGS/ref=sr_1_5?crid=1XQQR6O97M8TT&keywords=Micro+Mini+N20&qid=1692973074&sprefix=micro+mini+n20%2Caps%2C55&sr=8-5 "https://www.amazon.co.uk/HALJIA-300RPM-Reducing-Reduction-Terminals/dp/B07YB76TGS/ref=sr_1_5?crid=1XQQR6O97M8TT&keywords=Micro+Mini+N20&qid=1692973074&sprefix=micro+mini+n20%2Caps%2C55&sr=8-5")
    - [Female DC Power adapter - 2.1mm jack to screw terminal block](https://thepihut.com/products/female-dc-power-adapter-2-1mm-jack-to-screw-terminal-block "https://thepihut.com/products/female-dc-power-adapter-2-1mm-jack-to-screw-terminal-block")
    - Wires
    - Ball bearing (6mm x 16mm x 6mm)
    - Wooden Dowel Pins (6mm x 30mm)
- **Materials**
    - 1.8, 5, 10mm Transparent Acrylic Sheet
    - 1, 2.5, 5mm Wood Sheet
- **Machines**
    - Trotec Speedy 400 or equivalent
    - Prusa i3 MK3S or equivalent

3. **Hardware Setup**
- Navigate to `⁠models/final`
- Cut required components as per the name of the files.
- Assemble the primary structure using the laser-cut pieces.
- Integrate the sensors and ensure they are correctly calibrated.
- Connect the device to a power source.
2. **Web Application Setup**
- **Start from beginning**
    - Navigate to the `code/app/elow` directory.
    - Install necessary dependencies using `flutter pub get`.
    - Run the application using `flutter run`. (Tested in Edge, Android and iOS)
- or use pre-built files
    - Navigate to `⁠firebase/public` 
    - Upload all files to hosting platform

  

## Circuit Diagram
<p align="center"> <img src="https://raw.githubusercontent.com/Lionel-Lim/data-marble-machine/main/report/resource/circuit.png" width="500" alt="Circuit Diagram">


## Exhibition
The project is currently on public display at UCL East Urban Room.

<p align="center"> <img src="https://raw.githubusercontent.com/Lionel-Lim/data-marble-machine/main/report/resource/marblemachine_wide.jpeg" width="500" alt="Exhibited Project">

##