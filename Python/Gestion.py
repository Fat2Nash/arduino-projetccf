import json
import base64
import paho.mqtt.subscribe as subscribe
import mysql.connector

# Define database connection parameters
db_host = "Localhost"
port=3306,
db_user = "root"
db_password = ""
db_name = ""

def decode_frm_payload(payload_str):
    """Decodes the frm_payload assuming base64 encoding and UTF-8 string format."""
    try:
        decoded_payload_bytes = base64.b64decode(payload_str)
        decoded_payload_str = decoded_payload_bytes.decode("utf-8")
        return json.loads(decoded_payload_str)  # Assuming the payload is a JSON string
    except (UnicodeDecodeError, ValueError) as e:
        print(f"Error decoding payload: {e}")
        return None

def process_message(db, cursor, message):
    payload = json.loads(message.payload)
    print("Received message:")
    print("  Topic:", message.topic)
    print("  Payload (JSON):")
    print(json.dumps(payload, indent=4))

    # Check if frm_payload exists
    if "uplink_message" in payload and "frm_payload" in payload["uplink_message"]:
        frm_payload_str = payload["uplink_message"]["frm_payload"]
        print("  Decoded frm_payload:")
        decoded_payload = decode_frm_payload(frm_payload_str)
        print(decoded_payload)

        # Extract latitude and longitude
        if isinstance(decoded_payload, list) and len(decoded_payload) == 5:
            id_engin = decoded_payload[0]
            latitude = decoded_payload[1]
            longitude = decoded_payload[2]
            tempsfonctionnement = decoded_payload[3]
            alerte = decoded_payload[4]

            try:
                # Fetch the current location ID for the engin
                select_query = """
                  SELECT le.id_loc_engin, le.Temps_fonct
                  FROM loc_engin le
                  JOIN engins e ON e.id_engins = le.id_engins
                  WHERE e.id_engins = %s AND e.statut = 'Loué'
                  ORDER BY le.Louer_le DESC
                  LIMIT 1;
                """
                cursor.execute(select_query, (id_engin,))
                result = cursor.fetchone()

                if result:
                    id_loc_engin = result[0]
                    current_temps_fonct = result[1]
                    print(f"  Current location ID: {id_loc_engin}")  
                    print(f"  Current Temps_fonct: {current_temps_fonct}")
                    
                    # Check for existing entry to avoid duplicates
                    check_query = """
                      SELECT COUNT(*)
                      FROM position_engin
                      WHERE id_loc_engin = %s AND Longitude = %s AND Latitude = %s
                    """
                    cursor.execute(check_query, (id_loc_engin, longitude, latitude))
                    count_result = cursor.fetchone()
                    

                    if count_result[0] == 0:
                        
                        # Update Temps_fonct in loc_engin
                        update_query = """
                            UPDATE loc_engin
                            SET Temps_fonct = Temps_fonct + %s
                            WHERE id_loc_engin = %s;
                        """
                        cursor.execute(update_query, (tempsfonctionnement, id_loc_engin))
                        db.commit()
                        print(f"  Updated Temps_fonct in loc_engin: id_loc_engin={id_loc_engin}, Added Temps_fonct={tempsfonctionnement}")
                    
                        # Insert data into the position_engin table
                        insert_query = """
                          INSERT INTO position_engin (id_loc_engin, Longitude, Latitude, DateHeure)
                          VALUES (%s, %s, %s, NOW());
                        """
                        cursor.execute(insert_query, (id_loc_engin, longitude, latitude))
                        db.commit()
                        print(f"  Inserted into position_engin: id_loc_engin={id_loc_engin}, Longitude={longitude}, Latitude={latitude}")
                        

                        # Insert data into the alerte table if id_typeAlerte is not 0
                        if alerte != 0:
                            insert_alert_query = """
                                INSERT INTO alerte (id_engin, id_typeAlerte, status, date_alerte)
                                VALUES (%s, %s, "checkBatterie", NOW());
                            """
                            cursor.execute(insert_alert_query, (id_engin, alerte))
                            db.commit()
                            print(f"  Inserted into alerte: id_engin={id_engin}, id_typeAlerte={alerte}")
                        else:
                            print(f"  Alert ID is 0, not inserting into alerte table.")
                    else:
                        print(f"Duplicate entry found for id_loc_engin={id_loc_engin}, Longitude={longitude}, Latitude={latitude}")
                        
                        update_query = """
                            UPDATE loc_engin
                            SET Temps_fonct = Temps_fonct + %s
                            WHERE id_loc_engin = %s;
                        """
                        cursor.execute(update_query, (tempsfonctionnement, id_loc_engin))
                        db.commit()
                        print(f"  Updated Temps_fonct in loc_engin: id_loc_engin={id_loc_engin}, Added Temps_fonct={tempsfonctionnement}")
                        update_query = """
                            UPDATE engins
                            SET compteur_heures = compteur_heures + %s
                            WHERE id_engins = %s;
                        """
                        cursor.execute(update_query, (tempsfonctionnement, id_engin))
                        db.commit()
                        print(f"  Updated compteur_heures: id_engins={id_engin}, Added Temps_fonct={tempsfonctionnement}")
                        

                else:
                    print(f"No active location found for engin ID: {id_engin}")
                                            # Update Temps_fonct in loc_engin
                    update_query = """
                        UPDATE engins
                        SET compteur_heures = compteur_heures + %s
                        WHERE id_engins = %s;
                    """
                    cursor.execute(update_query, (tempsfonctionnement, id_engin))
                    db.commit()
                    print(f"  Updated compteur_heures: id_engins={id_engin}, Added Temps_fonct={tempsfonctionnement}")
                    

            except Exception as e:
                print(f"Error storing data: {e}")

def main():
    # MQTT connection details
    hostname = "eu1.cloud.thethings.network"
    port = 1883
    username = "thiriot-locations@ttn"
    password = "NNSXS.PKQO23GNU4C4BFCZII7KJGALOZCU4QSQAQYJBJI.FS4OL5BZZON7GOYQJGZOCV5JDNDPZTVVZJEWXE4IH5Y7BEUBA7XQ"

    while True:
        try:
            # Establish MySQL connection
            db = mysql.connector.connect(
                host=db_host,
                user=db_user,
                password=db_password,
                database=db_name,
                auth_plugin='caching_sha2_password'
            )
            cursor = db.cursor()

            # Subscribe to one message at a time
            message = subscribe.simple(topics=['#'], hostname=hostname, port=port, auth={'username': username, 'password': password}, msg_count=1)

            process_message(db, cursor, message)

        except Exception as e:
            print(f"Error connecting to the database or MQTT: {e}")
        finally:
            if db.is_connected():
                cursor.close()
                db.close()

if __name__ == "__main__":
    main()
