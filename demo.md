# 🚨 Emergency Lights Feature Documentation

The **Emergency Lights Feature** allows vehicles in the game to have **customizable emergency, hazard, and steady burn lights**. This documentation will guide you through the process of adapting vehicles to utilize this feature effectively.

## 📌 Getting Started

### 🔹 Siren Manifest Requirement
Every model with a siren **must** be configured with a JSON manifest. Without a JSON manifest, **ModelExtras** will not recognize the model as being adapted.

### 🔹 How the Siren Manifest Works
- The siren manifest is an **object** where the keys represent **different siren states**, toggled using the `R` key. Groups can be switched using keys `1-9`
- The **first key** in the object is always the **default state**.
- There is **no limit** to the number of states you can define.

---

# 📜 JSON Configuration Guide

## 1️⃣ **`references`**
Used to reference predefined configurations or templates stored elsewhere in the JSON. This allows for **reusability** and **modularity** in siren configurations.

### ✨ Example
```json
"colors": {
    "nearlyRed": { "red": 248, "green": 95, "blue": 95, "alpha": 160 },
    "OffWhite": { "red": 255, "green": 255, "blue": 255, "alpha": 160 },
    "lightblue": { "red": 52, "green": 255, "blue": 221, "alpha": 160 }
},
"customProperties": {
    "size": 0.3,
    "state": 1,
    "type": "rotator",
    "inertia": 1,
    "rotator": { "type": "linear" }
}
```
Now, these colors can be referenced anywhere in the JSON:
```json
"color": "lightblue"
```
Or even use the **customProperties reference**:
```json
"reference": "customProperties"
```
You can put this line inside any corona or shadow property to apply properties of this object to them.

---

## 2️⃣ **`size`**
Defines the **size** of the corona or shadow.
```json
"size": 1.5
```

---

## 3️⃣ **`diffuse`**
Controls the **diffuse properties** of the material.

### 🔧 Properties:
- `color`: (boolean) Enable/disable color.
- `transparent`: (boolean) Enable/disable transparency.

### ✨ Example
```json
{
  "diffuse": {
    "color": true,
    "transparent": false
  }
}
```

---

## 4️⃣ **`radius`**
Defines the **radius** of the siren corona.
```json
"radius": 10.0
```

---

## 5️⃣ **`color`**
Defines the **color** of the siren corona or shadow.

### 🔧 Behavior:
- If `color` is a **string**, it references a predefined color from `references`.
- If `color` is an **object**, it specifies the color using `red`, `green`, `blue`, and `alpha` values.

### ✨ Example
```json
"color": "lightBlue"
```
OR
```json
"color": {
  "red": 255,
  "green": 0,
  "blue": 0,
  "alpha": 255
}
```

---

## 6️⃣ **`state`**
Defines the **initial state** of the siren (on/off).
```json
"state": true
```

---

## 7️⃣ **`colors`**
Defines a sequence of **colors and timing** for dynamic siren effects.

### 🔧 Behavior:
- Each entry consists of:
  - A **delay** (time in milliseconds).
  - A **color** (either a string reference or an object with `red`, `green`, `blue`, `alpha` values).

### ✨ Example
```json
{
  "colors": [
    [1000, "lightBlue"],
    [1000, { "red": 255, "green": 0, "blue": 0, "alpha": 255 }]
  ]
}
```

---

## 8️⃣ **`pattern`**
Defines a **timing pattern** for siren flashing or rotation.

### 🔧 Behavior:
- **Array of numbers** → Each number represents a **delay in milliseconds**.
- **Array of arrays** → First element is the **number of iterations**, the rest are delays.

### ✨ Example
```json
"pattern": [500, 500, 1000]
```

---

## 9️⃣ **`type`**
Defines the **type** of siren (e.g., directional, non-directional, rotator).

### 🔧 Allowed Values:
- `"directional"` → Siren light is **directional**.
- `"non-directional"` → Siren light is **omnidirectional**.
- `"inversed-directional"` → Directional siren, but **inverted**.
- `"rotator"` → Siren **rotates**. (Rotator type can have a special object called `rotator` defining more properties for that type)

### ✨ Example
```json
  "type": "rotator",
  "rotator": {
    "time": 1000,
    "radius": 360,
    "direction": 0
  }
```

---

## 🔟 **`shadow`**
Defines **shadow properties** for the siren.

### 🔧 Properties:
- `size`: Shadow **size**.
- `type`: Shadow **type** (`"pointlight"`, `"narrow"`, `"round"`).
- `offset`: Shadow **offset**.

### ✨ Example
```json
  "shadow": {
    "size": 2.0,
    "type": "pointlight",
    "offset": 1.0
  }
```

---

## 1️⃣1️⃣ **`delay`**
Defines a **delay** before the siren effect starts.
```json
"delay": 1000
```

---

## 1️⃣2️⃣ **`inertia`**
Defines the **smoothness of transitions** between siren states.
```json
"inertia": 0.5
```

---

## 1️⃣3️⃣ **`ImVehFt`** (ModelExtras Only)
A special property signaling if the model was **adapted from ImVehFt**. Added during conversion from `.eml` to `.json`. **Do not add manually**.

### ✨ Example
```json
"ImVehFt": true
```
---

## ✨ 1️⃣4️⃣ **`Complete Example`**
```json
{
    "references": {
        "colors": {  // Defines a set of named colors with RGBA values
            "red":    { "red": 248, "green": 95, "blue": 95, "alpha": 160 },
            "white":  { "red": 255, "green": 255, "blue": 255, "alpha": 160 },
            "yellow": { "red": 255, "green": 149, "blue": 0, "alpha": 160 },
            "blueTint": { "red": 98, "green": 156, "blue": 236, "alpha": 160 },
            "lightblue": { "red": 52, "green": 255, "blue": 221, "alpha": 160 }
        },

        "myReference": {  // Defines a reusable myReference object
            "size": 0.3,  // Defines the size of the rotator
            "state": 1,  // Initial state of the rotator
            "type": "rotator", // Type identifier
            "inertia": 1, // Defines how smoothly it rotates
            "rotator": {
                "type": "linear" // Specifies the rotation type
            }
        }
    },

    "states": {
        // Defines each siren group, you can rotate between which group is active using hotkeys
        "FirstGroup": {  // Defines group 1 of sirens, can be named anything
            // These siren numbers need to match the dummies inside the model
            "1": {
                "color": "lightblue",  // Uses predefined color
                "reference": "myReference",  // References the myReference definition
                "rotator": { "time": 800 } // Defines rotation speed
            },

            "2": {
                "color": "lightblue",
                "size": 0.3,
                "inertia": 1.5,  // Higher inertia for smoother transitions
                "state": 1, // Active state
                "pattern": [ 300,300 ] // Defines the blink pattern (in milliseconds)
            },

            "3": {
                "color": "white",
                "reference": "myReference",
                "rotator": {
                    "direction": 0, 
                    "radius": 70.0, // Defines rotation radius
                    "offset": -35.0, // Offset angle
                    "time": 600 // Time duration for one full cycle
                }
            },

            "4": {
                "color": "blueTint",
                "size": 0.3,
                "state": 1,
                "pattern": [ 50,50,50,50,50,300,50,50,50,50,50,300 ] // Complex blink pattern
            },
        },
        "Group 2": { // This is a totally separate group 
          "15": {
			"size": 0.4,
			"color": {
				"red": 255,
				"green": 255,
				"blue": 255,
				"alpha": 255
			},
			"shadow": {
				"offset": 0.0,

				"size": 1.0,
	
				"type": 11
			},
			"state": 1,
			"pattern": []
		},

		"16": {
			"size": 0.4,
			"color": {
				"red": 255,
				"green": 255,
				"blue": 255,
				"alpha": 255
			},
			"shadow": {
				"offset": 0.0,

				"size": 1.0,
	
				"type": 11
			},
			"state": 1,
			"pattern": []
		}
        }
    }
}
```
---

## 🎉 Conclusion
This documentation provides a **comprehensive guide** on configuring emergency lights in **ModelExtras** using JSON. By following these guidelines, you can create **custom and dynamic** emergency lighting setups for in-game vehicles!

🚗💨 **Happy Modding!** 🎮
