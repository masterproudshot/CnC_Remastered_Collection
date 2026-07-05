//
// Ore / gem / mine placement — large clustered pockets (terrain-aware spawn clear).
//
using MobiusEditor.Model;
using MobiusEditor.RedAlert;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;

namespace MobiusEditor.MapGen
{
    public static class ResourcePlacement
    {
        private static readonly string[] GoldOverlayNames = { "gold01", "gold02", "gold03", "gold04" };
        private static readonly string[] GemOverlayNames = { "gem01", "gem02", "gem03", "gem04" };

        public sealed class Options
        {
            public double OreDensity { get; set; } = 0.72;
            public double GemDensity { get; set; } = 0.06;
            public int MineCount { get; set; } = 16;
            public int SpawnClearRadius { get; set; } = 5;
            public int OrePatchCount { get; set; } = 18;
            public int MinOreCellsPerPatch { get; set; } = 64;
            public int MaxOreCellsPerPatch { get; set; } = 320;
            public int GemPatchCount { get; set; } = 10;
            public int MinGemCellsPerPatch { get; set; } = 14;
            public int MaxGemCellsPerPatch { get; set; } = 72;
            public double OreSpreadChance { get; set; } = 0.86;
            public double GemSpreadChance { get; set; } = 0.78;
        }

        public static void Apply(
            Map map,
            Random random,
            double oreDensity,
            double gemDensity,
            int mineCount,
            IReadOnlyList<Point> spawnTiles,
            int spawnClearRadius)
        {
            Apply(map, random, spawnTiles, new Options
            {
                OreDensity = oreDensity,
                GemDensity = gemDensity,
                MineCount = mineCount,
                SpawnClearRadius = spawnClearRadius
            });
        }

        public static void Apply(Map map, Random random, IReadOnlyList<Point> spawnTiles, Options options)
        {
            if (options == null)
            {
                throw new ArgumentNullException(nameof(options));
            }

            var bounds = map.Bounds;
            var candidates = new List<Point>();
            foreach (var p in EnumerateRect(bounds))
            {
                if (IsBlockedForPlacement(map, p, spawnTiles, options.SpawnClearRadius))
                {
                    continue;
                }
                candidates.Add(p);
            }

            if (candidates.Count == 0)
            {
                return;
            }

            int oreTarget = (int)Math.Round(candidates.Count * Clamp01(options.OreDensity));
            int gemTarget = (int)Math.Round(candidates.Count * Clamp01(options.GemDensity));

            PlaceClusteredOre(map, random, candidates, oreTarget, spawnTiles, options);
            PlaceClusteredGems(map, random, candidates, gemTarget, spawnTiles, options);
            PlaceMines(map, random, options.MineCount, spawnTiles, options.SpawnClearRadius);
        }

        private static void PlaceClusteredOre(
            Map map,
            Random random,
            List<Point> candidates,
            int target,
            IReadOnlyList<Point> spawnTiles,
            Options options)
        {
            if (target <= 0 || candidates.Count == 0)
            {
                return;
            }

            var oreCells = new HashSet<int>();
            int patchCount = Math.Max(6, options.OrePatchCount);
            int perPatchBudget = Math.Max(options.MinOreCellsPerPatch, target / patchCount);

            for (int s = 0; s < patchCount && oreCells.Count < target; s++)
            {
                var seed = candidates[random.Next(candidates.Count)];
                int patchGoal = Math.Min(options.MaxOreCellsPerPatch, perPatchBudget + random.Next(0, perPatchBudget / 3));
                GrowResourcePatch(
                    map, random, seed, patchGoal, oreCells, spawnTiles, options.SpawnClearRadius,
                    options.OreSpreadChance, setOverlay: (cell, r) => SetOreOverlay(map, cell, r));
            }

            // Top up only with small random fills if patches under-shot density target.
            int guard = 0;
            while (oreCells.Count < target && guard++ < target * 4)
            {
                var seed = candidates[random.Next(candidates.Count)];
                if (IsBlockedForPlacement(map, seed, spawnTiles, options.SpawnClearRadius))
                {
                    continue;
                }
                if (!map.Metrics.GetCell(seed, out int cell))
                {
                    continue;
                }
                if (oreCells.Contains(cell) || map.Overlay[cell] != null)
                {
                    continue;
                }
                oreCells.Add(cell);
                SetOreOverlay(map, cell, random);
            }
        }

        private static void PlaceClusteredGems(
            Map map,
            Random random,
            List<Point> candidates,
            int target,
            IReadOnlyList<Point> spawnTiles,
            Options options)
        {
            if (target <= 0 || candidates.Count == 0)
            {
                return;
            }

            var gemCells = new HashSet<int>();
            int patchCount = Math.Max(3, options.GemPatchCount);
            int perPatchBudget = Math.Max(options.MinGemCellsPerPatch, target / patchCount);

            for (int s = 0; s < patchCount && gemCells.Count < target; s++)
            {
                var seed = candidates[random.Next(candidates.Count)];
                int patchGoal = Math.Min(options.MaxGemCellsPerPatch, perPatchBudget + random.Next(0, perPatchBudget / 2));
                GrowResourcePatch(
                    map, random, seed, patchGoal, gemCells, spawnTiles, options.SpawnClearRadius + 2,
                    options.GemSpreadChance, setOverlay: (cell, r) => SetGemOverlay(map, cell, r));
            }
        }

        private static void GrowResourcePatch(
            Map map,
            Random random,
            Point seed,
            int patchGoal,
            HashSet<int> filledCells,
            IReadOnlyList<Point> spawnTiles,
            int spawnClearRadius,
            double spreadChance,
            Action<int, Random> setOverlay)
        {
            var queue = new Queue<Point>();
            queue.Enqueue(seed);
            int placedInPatch = 0;
            int guard = 0;

            while (queue.Count > 0 && placedInPatch < patchGoal && guard++ < 20000)
            {
                var p = queue.Dequeue();
                if (IsBlockedForPlacement(map, p, spawnTiles, spawnClearRadius))
                {
                    continue;
                }
                if (!map.Metrics.GetCell(p, out int cell))
                {
                    continue;
                }
                if (filledCells.Contains(cell) || map.Overlay[cell] != null)
                {
                    continue;
                }

                filledCells.Add(cell);
                setOverlay(cell, random);
                placedInPatch++;

                foreach (var n in Neighbors8(p))
                {
                    if (!map.Bounds.Contains(n))
                    {
                        continue;
                    }
                    if (random.NextDouble() < spreadChance)
                    {
                        queue.Enqueue(n);
                    }
                }
            }
        }

        private static void PlaceMines(
            Map map,
            Random random,
            int mineCount,
            IReadOnlyList<Point> spawnTiles,
            int spawnClearRadius)
        {
            if (mineCount <= 0)
            {
                return;
            }

            var mineType = map.TerrainTypes.FirstOrDefault(t => t.Equals("mine"));
            if (mineType == null)
            {
                return;
            }

            var scores = new List<(int cell, int score)>();
            foreach (var (cell, overlay) in map.Overlay)
            {
                if (overlay == null || !overlay.Type.IsTiberiumOrGold)
                {
                    continue;
                }
                if (!map.Metrics.GetLocation(cell, out Point loc))
                {
                    continue;
                }
                if (IsBlockedForPlacement(map, loc, spawnTiles, spawnClearRadius))
                {
                    continue;
                }
                scores.Add((cell, LocalOreDegree(map, cell)));
            }

            foreach (var entry in scores.OrderByDescending(x => x.score).Take(mineCount))
            {
                if (!map.Metrics.GetLocation(entry.cell, out Point loc))
                {
                    continue;
                }
                if (map.Technos[loc] != null)
                {
                    continue;
                }
                map.Technos.Add(entry.cell, new Terrain
                {
                    Type = mineType,
                    Icon = 0,
                    Trigger = Trigger.None
                });
            }
        }

        private static int LocalOreDegree(Map map, int cell)
        {
            if (!map.Metrics.GetLocation(cell, out Point loc))
            {
                return 0;
            }
            int degree = 0;
            foreach (var n in Neighbors8(loc))
            {
                if (!map.Metrics.GetCell(n, out int nc))
                {
                    continue;
                }
                var o = map.Overlay[nc];
                if (o != null && o.Type.IsTiberiumOrGold)
                {
                    degree++;
                }
            }
            return degree;
        }

        private static void SetOreOverlay(Map map, int cell, Random random)
        {
            var goldType = map.OverlayTypes.FirstOrDefault(t => t.Equals(GoldOverlayNames[random.Next(GoldOverlayNames.Length)]));
            if (goldType == null)
            {
                return;
            }
            map.Overlay[cell] = new Overlay { Type = goldType, Icon = 0 };
        }

        private static void SetGemOverlay(Map map, int cell, Random random)
        {
            var gemType = map.OverlayTypes.FirstOrDefault(t => t.Equals(GemOverlayNames[random.Next(GemOverlayNames.Length)]));
            if (gemType == null)
            {
                return;
            }
            map.Overlay[cell] = new Overlay { Type = gemType, Icon = 0 };
        }

        private static bool IsBlockedForPlacement(
            Map map,
            Point p,
            IReadOnlyList<Point> spawnTiles,
            int spawnClearRadius)
        {
            if (IsNearAnySpawn(p, spawnTiles, spawnClearRadius))
            {
                return true;
            }

            return TerrainPlacement.IsBlockedForResources(map, p);
        }

        private static bool IsNearAnySpawn(Point p, IReadOnlyList<Point> spawnTiles, int radius)
        {
            foreach (var s in spawnTiles)
            {
                if (Math.Abs(p.X - s.X) <= radius && Math.Abs(p.Y - s.Y) <= radius)
                {
                    return true;
                }
            }
            return false;
        }

        private static IEnumerable<Point> Neighbors8(Point p)
        {
            yield return new Point(p.X - 1, p.Y);
            yield return new Point(p.X + 1, p.Y);
            yield return new Point(p.X, p.Y - 1);
            yield return new Point(p.X, p.Y + 1);
            yield return new Point(p.X - 1, p.Y - 1);
            yield return new Point(p.X + 1, p.Y - 1);
            yield return new Point(p.X - 1, p.Y + 1);
            yield return new Point(p.X + 1, p.Y + 1);
        }

        private static IEnumerable<Point> EnumerateRect(Rectangle rect)
        {
            for (int y = rect.Top; y < rect.Bottom; y++)
            {
                for (int x = rect.Left; x < rect.Right; x++)
                {
                    yield return new Point(x, y);
                }
            }
        }

        private static double Clamp01(double v) => v < 0 ? 0 : (v > 1 ? 1 : v);
    }
}