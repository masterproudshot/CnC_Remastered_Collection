//
// Writes .tga + .json sidecars next to .mpr (Remastered custom map browser previews).
//
using MobiusEditor.Interface;
using MobiusEditor.Model;
using MobiusEditor.RedAlert;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;

namespace MobiusEditor.MapGen
{
    public static class RedAlertMapSidecars
    {
        private const long MinTgaBytes = 4096;

        public static IList<string> WriteSidecars(Map map, string mprPath)
        {
            var errors = new List<string>();
            if (map == null)
            {
                errors.Add("Map is null.");
                return errors;
            }
            if (string.IsNullOrWhiteSpace(mprPath))
            {
                errors.Add("mprPath is empty.");
                return errors;
            }

            var tgaPath = Path.ChangeExtension(mprPath, ".tga");
            var jsonPath = Path.ChangeExtension(mprPath, ".json");

            try
            {
                var preview = map.GenerateMapPreview();
                using (var fs = new FileStream(tgaPath, FileMode.Create, FileAccess.Write, FileShare.Read))
                {
                    if (!preview.Save(fs))
                    {
                        errors.Add("TGA Save returned false for " + tgaPath);
                    }
                    fs.Flush(true);
                }
            }
            catch (Exception ex)
            {
                errors.Add("TGA exception: " + ex.Message);
            }

            try
            {
                WriteJson(map, jsonPath);
            }
            catch (Exception ex)
            {
                errors.Add("JSON exception: " + ex.Message);
            }

            if (File.Exists(tgaPath) && new FileInfo(tgaPath).Length < MinTgaBytes)
            {
                errors.Add("TGA too small after write: " + tgaPath);
            }
            if (File.Exists(jsonPath) && new FileInfo(jsonPath).Length < 20)
            {
                errors.Add("JSON too small after write: " + jsonPath);
            }

            return errors;
        }

        public static IList<string> RepairMpr(string mprPath)
        {
            var errors = new List<string>();
            if (!File.Exists(mprPath))
            {
                errors.Add("MPR not found: " + mprPath);
                return errors;
            }

            using (var plugin = new GamePlugin(mapImage: false))
            {
                errors.AddRange(plugin.Load(mprPath, FileType.INI));
                if (errors.Count > 0)
                {
                    return errors;
                }
                errors.AddRange(WriteSidecars(plugin.Map, mprPath));
            }

            return errors;
        }

        private static void WriteJson(Map map, string jsonPath)
        {
            using (var fs = new FileStream(jsonPath, FileMode.Create, FileAccess.Write, FileShare.Read))
            using (var sw = new StreamWriter(fs))
            using (var writer = new JsonTextWriter(sw))
            {
                writer.WriteStartObject();
                writer.WritePropertyName("MapTileX");
                writer.WriteValue(map.MapSection.X);
                writer.WritePropertyName("MapTileY");
                writer.WriteValue(map.MapSection.Y);
                writer.WritePropertyName("MapTileWidth");
                writer.WriteValue(map.MapSection.Width);
                writer.WritePropertyName("MapTileHeight");
                writer.WriteValue(map.MapSection.Height);
                writer.WritePropertyName("Theater");
                writer.WriteValue(map.MapSection.Theater.Name.ToUpper());
                writer.WritePropertyName("Waypoints");
                writer.WriteStartArray();
                foreach (var waypoint in map.Waypoints)
                {
                    if (waypoint.Flag == WaypointFlag.PlayerStart && waypoint.Cell.HasValue)
                    {
                        writer.WriteValue(waypoint.Cell.Value);
                    }
                }
                writer.WriteEndArray();
                writer.WriteEndObject();
                writer.Flush();
                sw.Flush();
                fs.Flush(true);
            }
        }
    }
}